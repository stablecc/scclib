# build and OpenSSL development packages

On Debian-like distributions:
```
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install libssl-dev
```

On Fedora/Red Hat-like distributions:
```
sudo yum update
sudo yum groupinstall "Development Tools"
sudo yum install openssl-devel
```

On Gentoo:
```
sudo emerge -a dev-libs/openssl
```
