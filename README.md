# Simple Capture Tool
A Simple Screenshot Tool for Linux

<br>

## Compile Instructions

1. **Download SCT Repo**
 ```
   git clone https://github.com/zszillat/simplecapturetool
```
2. **Download Dependencies**
```
[Debian]
sudo apt-get update
sudo apt-get install -y libx11-dev libxext-dev libpng-dev
[Arch]
sudo pacman -Sy
sudo pacman -S libx11 libxext libpng
```
3. **Compile**
```
gcc sct.c -o sct -lX11 -lXext -lpng
```
