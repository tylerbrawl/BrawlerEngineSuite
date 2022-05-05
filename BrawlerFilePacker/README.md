# Brawler File Packer

## Description
This is the program used to pack loose asset files into the .bpk archive which is consumed by the Brawler Engine. It is a command line application.

## Usage
The following describes the usage of the Brawler File Packer:

`BrawlerFilePacker.exe [Root Directory of Source Asset Files] [Root Output Directory] [Additional Switches (Optional)]`

The following switches are currently supported:
* Debug Mode Build: `/D` - Compresses data files using a zstandard compression level suitable for Debug builds. Files will compress *MUCH* faster, but with a smaller compression ratio and slower decompression time.
* Release Mode Build: `/R` - Compresses data files using a zstandard compression level suitable for Release builds. Files will compress *MUCH* slower, but with a larger compression ratio and faster decompression time. This is the default setting if neither /D nor /R is specified.

## Additional Information
For each source asset, a Brawler Compiled Asset (BCA) file is made in the root output directory under the `Asset Cache` folder. The directory tree for each BCA file resembles that of the source asset which it represents.

The BCA files are used to speed up asset compilation times when no changes have been made to an asset since the last build. They are *NOT* needed at runtime, and should *NOT* be distributed to users.

Compression is done using the [zstandard](https://github.com/facebook/zstd) library.
