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

var ErrInit = errors.New("wayland init failed")

func Init(path string, width, height int, output string) error {
	cpath := C.CString(path)
	coutput := C.CString(output)
	defer C.free(unsafe.Pointer(cpath))
	defer C.free(unsafe.Pointer(coutput))

	if C.wl_init(C.int(width), C.int(height), cpath, coutput) != 0 {
		return ErrInit
	}
	return nil
}

func Show() {
	C.wl_show()
}

func Hide() {
	C.wl_hide()
}

func Destroy() {
	C.wl_destroy()
}

func Visible() bool {
	return C.cross_visible() != 0
}
