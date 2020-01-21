# png2c - Oleg Vaskevich
png2c: ImgEmwinConverter.c
	gcc -o ImgEmwinConverter ImgEmwinConverter.c -I. -lpng

clean:
	rm -f ImgEmwinConverter

