all: tema4.cpp
	mpiCC tema4.cpp -o tema4
run: tema4
	mpirun -n 12 ./tema4 topologie.txt mesaje.txt
	awk 'FNR==1{print ""}1' rank*.txt > etapa1.txt
run2: tema4
	mpirun -n 12 ./tema4 topologie2.txt mesaje.txt
	awk 'FNR==1{print ""}1' rank*.txt > etapa1.txt

clean:
	rm tema4 rank*.txt
