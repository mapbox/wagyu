#!/usr/bin/env bash

MASON_VERSION="b709931"

function setup_mason() {
    if [[ ! -d ./.mason ]]; then
        git clone https://github.com/mapbox/mason.git ./.mason
        (cd ./.mason && git checkout ${MASON_VERSION})
    else
        echo "Updating to latest mason"
        (cd ./.mason && git fetch && git checkout ${MASON_VERSION})
    fi
    export PATH=$(pwd)/.mason:$PATH
}

function install_clang() {
    mason install clang 3.8.0
    export PATH=$(mason prefix clang 3.8.0)/bin:${PATH}
    export CXX=clang++
    export CC=clang
}

function main() {
    BOOTSTRAP=${MASON_CLANG:-false}
    if $BOOTSTRAP; then
        setup_mason
        install_clang
    fi
}

main
