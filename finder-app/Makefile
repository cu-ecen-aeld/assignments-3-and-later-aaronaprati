
CROSS_COMPILE=""

writer: writer.o
	@if [ -z "$(CROSS_COMPILE)" ];\
	then\
		gcc -o writer writer.o;\
	else\
		"$(CROSS_COMPILE)"gcc -o writer writer.o;\
	fi;\

writer.o: writer.c
	@"$(CROSS_COMPILE)"gcc -c writer.c

clean:
	@rm writer writer.o
