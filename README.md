# Simple Capture Tool
A Simple Screenshot Tool for Linux

## Usage
```
  -h, --help       Prints out instructions
  -n, --name       Specify a filename
  -p, --path       Specify a path, e.g., "home/username/Documents"
  -c, --clipboard  Copies the image to clipboard
  -s, --save       Saves file to either home folder or specified folder
```
<br>

## Compile Instructions

1. **Download SCT Repo**
 ```
   git clone https://github.com/zszillat/simplecapturetool
```
2. **Download Dependencies**

###### Debian
```
sudo apt-get update
sudo apt-get install -y libx11-dev libxext-dev libpng-dev
```
###### Arch
```
sudo pacman -Sy
sudo pacman -S libx11 libxext libpng
```
3. **Compile**
```
cd simplecapturetool
gcc sct.c -o sct -lX11 -lXext -lpng
sudo cp sct /usr/local/bin #Add to path (optional)
```
4. **Clean Up**
```
cd ..
rm -rf simplecapturetool
```

