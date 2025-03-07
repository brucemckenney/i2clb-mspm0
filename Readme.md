This is a sample/skeleton for I2C for the MSPM0 series.

As packaged, it contains a Master (i2cm) and a Slave (i2cs), suitable for cross-connection using patch wires. Each component is designed to operate standalone, so each driver can be imported into a different project by copying the .c/.h/_conf.h files.

The Master talks to a "register" model slave using register reads and writes (1-byte register number). It could be used with any other I2C slave device that implements the register model (that's most of them). After i2cm_init(), main() may call into the driver to read and write registers.

The Slave implements a register array as a memory array. Registers can be read and written, but no added function is provided. This could be used as a skeleton to which useful function can be added. It is interrupt-driven, and so runs in the background after a call to i2cs_init().

These do not depend on sysconfig/driverlib. The _conf.h files serve the purpose of the DL_config.h files, but aren't rewritten.
