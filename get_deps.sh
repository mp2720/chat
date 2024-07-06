#!/bin/bash
set -e

IMGUI_VERSION=1.90.8

# args: name version repo_name copy_meson_build(true/false)
function download_github_tar_gz_release {
    name=$1
    version=$2
    url="https://github.com/$3/archive/refs/tags/v$version.tar.gz"
    tar_gz="$1.tar.gz"
    copy_meson_build=$4

    rm -rf "$name"
    curl -L -f "$url" --output "$tar_gz"
    tar -xzf "$tar_gz"
    rm -rf "$tar_gz"
    mv "$name-$version" "$name"
    if [ "$copy_meson_build" = "true" ] ; then
        cp "$name.build" "$name/meson.build"
    fi
}

cd subprojects

download_github_tar_gz_release "imgui" "1.90.8" "ocornut/imgui" true
download_github_tar_gz_release "utfcpp" "4.0.5" "nemtrif/utfcpp"

echo "ok"
