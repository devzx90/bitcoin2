
Debian
====================
This directory contains files used to package bitcoin2d/bitcoin2-qt
for Debian-based Linux systems. If you compile bitcoin2d/bitcoin2-qt yourself, there are some useful files here.

## bitcoin2: URI support ##


bitcoin2-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bitcoin2-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bitcoin2qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin2128.png` to `/usr/share/pixmaps`

bitcoin2-qt.protocol (KDE)

