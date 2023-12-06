#!/bin/bash

# clean previous generated files(if any) and build
echo "Removing previous autogen files:" && \
    autoreconf --verbose --instal