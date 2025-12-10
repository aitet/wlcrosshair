package config

import (
	"errors"
	"fmt"
	"os"
	"github.com/BurntSushi/toml"
)

type Config struct {
	Path    string
	Width   int
	Height  int
	Output  string
}

type rawConfig struct {
	Path    string `toml:"path"`
	Size    int    `toml:"size"`
	Width   int    `toml:"width"`
	Height  int    `toml:"height"`
	Output  string `toml:"output"`
}

func Load(path string) (*Config, error) {
	if _, err := os.Stat(path); err != nil {
		return nil, fmt.Errorf("%w", err)
	}

	var raw rawConfig
	if _, err := toml.DecodeFile(path, &raw); err != nil {
		return nil, err
	}

	if raw.Path == "" {
		return nil, errors.New("'path' is required")
	}

	if raw.Output == "" {
		return nil, errors.New("'output' is required")
	}

	if raw.Size > 0 {
		return &Config{
			Path:   raw.Path,
			Width:  raw.Size,
			Height: raw.Size,
			Output: raw.Output,
		}, nil
	}

	if raw.Width > 0 && raw.Height > 0 {
		return &Config{
			Path:   raw.Path,
			Width:  raw.Width,
			Height: raw.Height,
			Output: raw.Output,
		}, nil
	}

	return nil, errors.New(
		"either 'size' or both 'width' and 'height' are required",
	)
}
