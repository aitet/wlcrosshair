package ipc

import (
	"bufio"
	"net"
	"os"
	"strings"
)

type Handler func(cmd string)

func Listen(path string, h Handler) error {
	_ = os.Remove(path)

	l, err := net.Listen("unix", path)
	if err != nil {
		return err
	}

	go func() {
		for {
			c, err := l.Accept()
			if err != nil {
				continue
			}
			go handleConn(c, h)
		}
	}()

	return nil
}

func handleConn(c net.Conn, h Handler) {
	defer c.Close()
	s := bufio.NewScanner(c)
	if s.Scan() {
		h(strings.TrimSpace(s.Text()))
	}
}
