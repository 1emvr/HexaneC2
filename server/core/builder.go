package core

import (
	"fmt"
	"os"
	"path"
	"path/filepath"
	"time"
)

const (
	BUILD_TYPE_SHELLCODE = 0
	BUILD_TYPE_DLL       = 1
)

var (
	BuildPath   = path.Join(RootDirectory, "build")
	ConfigsPath = path.Join(RootDirectory, "configs")
	CorePath    = path.Join(RootDirectory, "core")
	LogsPath    = path.Join(RootDirectory, "logs")
	LoaderPath  = path.Join(RootDirectory, "loader")
	ImplantPath = path.Join(RootDirectory, "implant")
	HashHeader  = path.Join(CorePath, "include/names.hpp")
	HashStrings = path.Join(ConfigsPath, "strings.txt")
)

func (h *HexaneConfig) BuildModule() error {
	var err error

	builder := h.UserConfig.Builder
	if builder.RootDirectory == "" {
		return fmt.Errorf("source directory is required")
	}

	if builder.OutputName == "" {
		return fmt.Errorf("output name is required")
	}

	if builder.LinkerScript != "" {
		builder.LinkerScript = filepath.Join(builder.RootDirectory, builder.LinkerScript)
	}

	if err = h.BuildSources(); err != nil {
		return err
	}

	if builder.Files.Dependencies != nil {
		builder.Components = append(builder.Components, builder.Files.Dependencies...)
	}

	if len(builder.Components) > 1 {
		return h.ExecuteBuildType(builder)
	} else {
		builder.OutputName = builder.Components[0]
		return nil
	}
}

func (h *HexaneConfig) RunBuild() error {
	var err error

	WrapMessage("DBG", "creating payload directory")
	if err = os.MkdirAll(h.Compiler.BuildDirectory, os.ModePerm); err != nil {
		return err
	}

	WrapMessage("DBG", "generating config")
	if err = h.GenerateConfigBytes(); err != nil {
		return err
	}

	WrapMessage("DBG", "generating hashes")
	if err = GenerateHashes(HashStrings, HashHeader); err != nil {
		return err
	}

	WrapMessage("DBG", "generating implant\n")
	if err = h.BuildModule(); err != nil {
		return err
	}

	WrapMessage("DBG", "embedding implant config data")
	if err = h.EmbedSectionData(BuildPath+"/implant.exe", ".text$F", h.ConfigBytes); err != nil {
		return err
	}

	WrapMessage("DBG", "extracting shellcode")
	if err = h.CopySectionData(BuildPath+"/implant.exe", path.Join(h.Compiler.BuildDirectory, "shellcode.bin"), ".text"); err != nil {
		return err
	}

	go func() {
		err = h.HttpServerHandler()
	}()

	time.Sleep(500 * time.Millisecond)
	if err != nil {
		return err
	}

	AddConfig(h)
	WrapMessage("INF", fmt.Sprintf("%s ready!", h.UserConfig.Builder.OutputName))

	return nil
}
