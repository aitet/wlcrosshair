package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"

	"github.com/marzeq/wlcrosshair/internal/config"
	"github.com/marzeq/wlcrosshair/internal/ipc"
	"github.com/marzeq/wlcrosshair/internal/shared"
	"github.com/marzeq/wlcrosshair/internal/wl"
)

func main() {
	runtime.LockOSThread()

	configPath := os.Getenv("XDG_CONFIG_HOME")
	if configPath == "" {
		configPath = os.Getenv("HOME") + "/.config"
	}
	if configPath == "" {
		fmt.Fprintf(os.Stderr, "Could not determine config path\n")
		os.Exit(1)
	}

	configPath += "/wlcrosshair/config.toml"

	conf, err := config.Load(configPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to load config: %v\n", err)
		os.Exit(1)
	}

	if err := wl.Init(
		expandUser(conf.Path),
		conf.Width,
		conf.Height,
		conf.Output,
	); err != nil {
		os.Exit(1)
	}

	ipc.Listen(shared.SocketPath, func(cmdStr string) {
		cmd, err := shared.ValidateCommand(cmdStr)
		if err != nil {
			return
		}

		switch cmd {
		case shared.CmdToggle:
			if wl.Visible() {
				wl.Hide()
			} else {
				wl.Show()
			}
		case shared.CmdShow:
			wl.Show()
		case shared.CmdHide:
			wl.Hide()
		case shared.CmdQuit:
			wl.Destroy()
			os.Exit(0)
		}
	})

	select {}
}

func expandUser(path string) string {
	if path == "" || path[0] != '~' {
		return path
	}

	home, err := os.UserHomeDir()
	if err != nil {
		return path
	}

	if path == "~" {
		return home
	}

	if strings.HasPrefix(path, "~/") {
		return filepath.Join(home, path[2:])
	}

	return path
}
