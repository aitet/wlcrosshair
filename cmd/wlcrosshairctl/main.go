package main

import (
	"fmt"
	"net"
	"os"

	"github.com/marzeq/wlcrosshair/internal/shared"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: wlcrosshairctl <command>\n")
		os.Exit(1)
	}

	if _, err := shared.ValidateCommand(os.Args[1]); err != nil {
		fmt.Fprintf(os.Stderr, "Invalid command: %v\n", err)
		os.Exit(1)
	}

	c, err := net.Dial("unix", shared.SocketPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to connect to daemon - is it running?\n")
		os.Exit(1)
	}
	defer c.Close()

	c.Write([]byte(os.Args[1] + "\n"))
}
