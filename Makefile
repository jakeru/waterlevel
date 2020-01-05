default: build

build:
	pio run

uploadfs:
	pio run --target uploadfs

upload:
	pio run --target upload
