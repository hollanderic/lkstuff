MODULES += \
	app/shell \
 	app/tests \
 	dev/sensor/pressure/bmp280 \

GLOBAL_DEFINES += \
	BMP280_DEBUG_LOG=1 \

include project/target/nrf-pca10056.mk
