# wlcrosshair

Simple crosshair overlay for Wayland. 

## Supported compositors

Any compositor that implements the layer shell protocol should work, including:

- **Hyprland**
- wlroots based compositors like **Sway**
- Smithay based compositors like **COSMIC** & **niri**
- **KDE Plasma** Wayland

Notable compositors that do not implement the layer shell protocol and thus are not supported:

- **GNOME**

Of course, as this is based on Wayland technology, it will not work on X11.

## Usage

Create a configuration file at `~/.config/wlcrosshair/config.toml` to customize the crosshair appearance. Example configuration:

```toml
path = "~/.config/wlcrosshair/rotated.png" # REQUIRED: path to crosshair image
output = "DP-2" # REQUIRED: output name as named by your compositor
size = 24 # REQUIRED: size of the crosshair in pixels, will resize the image to size x size pixels
# OR if the image is not square, use width and height instead:
# width = 32 # width of the crosshair in pixels
# height = 16 # height of the crosshair in pixels
```

Run `wlcrosshair` in the background through a systemd user service or your compositor's autostart configuration.

Initially, the crosshair will not be visible. You can toggle its visibility using the following command:

```bash
wlcrosshairctl toggle
# or show/hide to force visibility
```

To close the crosshair overlay, either kill the `wlcrosshair` process or use:

```bash
wlcrosshairctl quit
```

If you do not have a crosshair image, you can find some samples I made [here](./sample_crosshairs/). You're also welcome to add your own ones to the repo.

## Installation

### Building from source:

```bash
git clone git@github.com:marzeq/wlcrosshair.git
cd wlcrosshair

go build -o wlcrosshair ./cmd/wlcrosshair
go build -o wlcrosshairctl ./cmd/wlcrosshairctl
```

Build dependencies:

- Go 1.25+
- Wayland development libraries

### Install from releases:

Grab the latest release binaries from the [releases page](https://github.com/marzeq/wlcrosshair/releases) and place them in your path.

### Install from package manager:

I'll make an AUR package soon enough. You're welcome to package it for your distro too.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## FAQ

#### Can you add a new feature?

No. I consider this project feature complete for my use case. Pull requests are welcome, but I will not be adding new features myself.

#### X11 support?

No. Let it die in peace.

#### GNOME support?

When they get their heads out of their asses and implement the protocol, of course.
