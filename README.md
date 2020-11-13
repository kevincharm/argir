# argir

This is my very slow-moving hobby kernel that I hack on in my spare time. Currently, nothing works! 😌

<img width="602" alt="Screen Shot 2020-11-13 at 6 30 29 pm" src="https://user-images.githubusercontent.com/10385659/99103890-8d646100-25e0-11eb-8348-5525451c7520.png" />

## Toolchain

- [docker](https://www.docker.com/products/docker-desktop) - The cross compiler and other build tools are pulled as a Docker image during build time for reproducibility.
- [qemu](https://www.qemu.org/download) - I develop this on QEMU. Of course, you can also just burn the ISO and boot it on real metal.
