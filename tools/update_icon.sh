#!/bin/sh
# Resize the supplied launcher artwork to the PSP's ICON0 dimensions.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
magick "$project_dir/docs/artwork/portrait-player-icon-master.png" -filter Lanczos -resize '144x80!' -strip "PNG24:$project_dir/ICON0.PNG"
