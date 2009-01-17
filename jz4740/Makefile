
OBJS	:= start.o main.o jz_serial.o

CROSS	:= mipsel-linux-

CFLAGS	:= -O2 -G 0 -mno-abicalls -fno-pic -mips32 -I../include -I../boards/onda_vx747
AFLAGS	= -D__ASSEMBLY__ $(CFLAGS)
LDFLAGS	:= -T ld.script -nostdlib -EL

.c.o:
	$(CROSS)gcc $(CFLAGS) -c $< -o $@
.S.o:
	$(CROSS)gcc $(AFLAGS) -c $< -o $@

jz_xloader.bin: jz_xloader
	$(CROSS)objdump -D jz_xloader $< > jz_xloader.dump
	$(CROSS)objcopy -O binary $< $@

jz_xloader: $(OBJS)
	$(CROSS)ld $(LDFLAGS) $^ -o $@

clean:
	rm -fr *.o jz_xloader jz_xloader.bin jz_xloader.dump
