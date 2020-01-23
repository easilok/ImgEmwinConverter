# png2c - Oleg Vaskevich
png2c: ImgEmwinConverter.c
	args = ImgEmwinConverter.c -I. -lpng
	gcc -o ImgEmwinConverter $(args)
	gcc -o ImgEmwinConverter.exe $(args)

clean:
	rm -f ImgEmwinConverter

