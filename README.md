# Karousel FX

KWin effects for [Karousel](https://github.com/peterfajdiga/karousel), the scrollable tiling
script. They also work without it.

## Effects

- **Focus ring:** highlights the active window with a colored outline that follows its shape.
- **Window motion:** animates windows when moved or resized by programs or scripts.

Window motion started out as a C++ port of Peter Fajdiga's [Geometry Change](https://github.com/peterfajdiga/kwin4_effect_geometry_change) and has been modified since then.

## Requirements

- KDE Plasma 6.7 or later.

## Install

### 1. Install the build dependencies

Fedora:

```sh
sudo dnf install cmake make gcc-c++ extra-cmake-modules kwin-devel kdecoration-devel kf6-kcolorscheme-devel kf6-kcmutils-devel kf6-kwidgetsaddons-devel qt6-qtdeclarative-devel wayland-devel libepoxy-devel libdrm-devel vulkan-headers vulkan-loader-devel
```

Arch:

```sh
sudo pacman -S --needed base-devel cmake extra-cmake-modules kwin kdecoration kcolorscheme kconfig kcoreaddons kcmutils kwidgetsaddons wayland libepoxy libdrm vulkan-headers
```

openSUSE:

```sh
sudo zypper install cmake make gcc-c++ kf6-extra-cmake-modules kwin6-devel kdecoration6-devel kf6-kcolorscheme-devel kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kcmutils-devel kf6-kwidgetsaddons-devel kf6-kwindowsystem-devel qt6-declarative-devel wayland-devel libepoxy-devel libdrm-devel vulkan-devel
```

Debian:

```sh
sudo apt install cmake make g++ pkg-config extra-cmake-modules kwin-dev libkdecorations3-dev libkf6colorscheme-dev libkf6config-dev libkf6coreaddons-dev libkf6kcmutils-dev libkf6widgetsaddons-dev libkf6windowsystem-dev qt6-declarative-dev libwayland-dev libepoxy-dev libdrm-dev libvulkan-dev
```

### 2. Build and install

```sh
make
sudo make install
```

Then open System Settings > Window Management > Desktop Effects and enable Karousel FX.

## Uninstall

```sh
sudo make uninstall
```
