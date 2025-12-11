package wl

/*
#cgo pkg-config: wayland-client libpng
#include <stdlib.h>
#include "layer.h"
*/
import "C"

import (
	"errors"
	"unsafe"
)

func InitSurface(width, height int, output string) error {
	coutput := C.CString(output)
	defer C.free(unsafe.Pointer(coutput))

	if !bool(C.init_surface(C.int(width), C.int(height), coutput)) {
		return errors.New("failed to initalise surface")
	}
	return nil
}

func LoadPNG(path string) bool {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	return bool(C.load_png(cpath))
}

func DrawPNG() {
	C.draw_png()
}

func ClearSurface() {
	C.clear_surface()
}

func FreePNG() {
	C.free_png()
}

func DestroySurface() {
	C.destroy_surface()
}
