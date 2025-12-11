package shared

import "errors"

const SocketPath = "/tmp/crosshair.sock"

type Command string

const (
	CmdToggle Command = "toggle"
	CmdShow   Command = "show"
	CmdHide   Command = "hide"
	CmdReload Command = "reload"
	CmdQuit   Command = "quit"
)

var AllCommands = map[Command]struct{}{
	CmdToggle: {},
	CmdShow:   {},
	CmdHide:   {},
	CmdReload: {},
	CmdQuit:   {},
}

func ValidateCommand(s string) (Command, error) {
	cmd := Command(s)
	if _, ok := AllCommands[cmd]; !ok {
		return "", errors.New("invalid ipc command")
	}
	return cmd, nil
}
