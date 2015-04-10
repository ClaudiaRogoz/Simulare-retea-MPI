#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <utility>
#include <algorithm>

#define ETAPA1 1
#define ETAPA2 2
#define ETAPA3 3
#define BUFFLEN 100

#define PTAG 3
#define VTAG 4

using namespace std;

int main(int argc, char *argv[]) {
	int  numtasks, rank, len, rc; 
	int i, j, k, parent = -1;
	MPI_Status Stat;
	int count = 0;
	FILE* fin = fopen(argv[1], "r");
	FILE* fmes = fopen (argv[2], "r");
	char buffer[100];
	bool frunza = false;
	rc = MPI_Init(&argc,&argv);
	time_t rawtime;
	struct tm* timeinfo;
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	int vecini[numtasks];
	int tab_rutare[numtasks] = {0};
	int aux[numtasks];	
	int line = 0;
	char *pch;
	int adj[numtasks*numtasks] = {0};
	/*toate procesele isi citesc lista de vecini din fisier*/
	while(!feof(fin)){
		fgets(buffer, BUFFLEN, fin);
		pch = strtok(buffer, " ");
		line = atoi(pch);
		if(line == rank){
			line = 0;
			pch = strtok(NULL, " -");
			while(pch != NULL){
				vecini[line++] = atoi(pch);
				pch = strtok(NULL, " ");
			}
			break;
		}
	}


	fgets(buffer, BUFFLEN, fmes);
	int nline = atoi(buffer);
	int kl;
	vector<pair<int, string> > message; 

	/*toate procesele isi citesc mesajele pe care le vor transmite*/
	for( kl= 0; kl< nline; kl++){
		fgets(buffer, BUFFLEN, fmes);
		pch = strtok(buffer, " ");
		k = atoi(pch);
		if(k == rank){
			pch = strtok(NULL, " ");
			/*daca este mesaj de broadcast*/
			if(pch[0] == 'B'){
				pch = strtok(NULL, "\n");
				message.push_back(make_pair(-1, pch));
			}
			else{
				k = atoi(pch);
				pch = strtok(NULL, "\n");
				message.push_back(make_pair(k, pch));
			}
		}
	}

	/*asteapta pana cand toate procesele se initializeaza*/
	MPI_Barrier(MPI_COMM_WORLD);


	/*daca este initiatorul*/
	if(rank == 0){


		/*trimite mesaje de sondaj pentru toti vecinii*/
		for(i = 0; i< line; i++){
			MPI_Send(&rank,1, MPI_INT, vecini[i], ETAPA1, MPI_COMM_WORLD);
		}

		int count_aux;
		/*asteapta apoi mesajele de la vecini*/
		for(i = 0; i< line; i++){
			MPI_Recv(aux, numtasks, MPI_INT, vecini[i], ETAPA1, MPI_COMM_WORLD, &Stat);
			MPI_Get_count(&Stat, MPI_INT, &count_aux);

			/*verifica daca isi updateaza propria tabela*/
			if(count_aux != 1){

				for(j = 0; j< numtasks; j++){
					if(aux[j] != 0){
						tab_rutare[j] = vecini[i];

					}

				}
			}
		}
		FILE *fp;
		fp = fopen("rank0.txt", "w");
		/*scrie tabela de rutare*/
		fprintf(fp, "%d\n", rank);
		for(i = 0; i< numtasks; i++){

			fprintf(fp, "%d %d\n", tab_rutare[i], i);
		}
		fclose(fp);
	}
	else{

		/*asteapta mesaje de la parinte*/
		MPI_Recv(&len, 1, MPI_INT, MPI_ANY_SOURCE, ETAPA1, MPI_COMM_WORLD, &Stat);

		/*se udateaza parintele*/
		parent = Stat.MPI_SOURCE;

		/*tabela de rutare initializata cu parintele*/
		for(i = 0; i< numtasks; i++){
			tab_rutare[i] = parent;
		}
		tab_rutare[rank] = rank;

		/*daca este nod frunza*/
		if(line == 1){
			frunza = true;

			/*trimite tabela de rutare parintelui*/
			MPI_Send(tab_rutare, numtasks, MPI_INT, parent, ETAPA1, MPI_COMM_WORLD);

			FILE *fp;
			char filename[30];
			sprintf(filename, "rank%d.txt", rank);
			fp = fopen(filename, "w");
			/*Scrie tabela de rutare*/
			fprintf(fp, "%d\n", rank);
			for(i = 0; i< numtasks; i++){

				fprintf(fp, "%d %d\n", tab_rutare[i], i);
			}
			fclose(fp);
		}

		else {

			/*trimite mesajul de sondaj mai departe*/
			for(i = 0; i< line; i++){
				if(vecini[i] != parent){
					MPI_Send(&len,1, MPI_INT, vecini[i], ETAPA1, MPI_COMM_WORLD);
				}
			}
			/*asteapta tabelele de rutare ale vecinilor
			  pentru propagarea catre parinte*/
			int count_aux;
			for(i = 0; i< line; i++){
				if(vecini[i] != parent){
					MPI_Recv(aux, numtasks, MPI_INT, vecini[i], ETAPA1, MPI_COMM_WORLD, &Stat);

					MPI_Get_count(&Stat, MPI_INT, &count_aux);

					/*verifica daca isi updateaza propria tabela*/
					if(count_aux != 1){
						for(j = 0; j< numtasks; j++){
							/*daca nu sunt copii directi
							 *se updateaza tabela*/
							if(aux[j] != rank){
								tab_rutare[j] = vecini[i];
							}		
						}
					}


				}
			}



			/*afisarea tabelei*/
			FILE *fp;
			char filename[30];
			sprintf(filename, "rank%d.txt", rank);
			fp = fopen(filename, "w");
			/*Scrie tabela de rutare*/
			fprintf(fp, "%d\n", rank);
			for(i = 0; i< numtasks; i++){

				fprintf(fp, "%d %d\n", tab_rutare[i], i);
			}
			fclose(fp);

			/*trimiterea tabelei inapoi la parinte*/
			MPI_Send(tab_rutare, numtasks, MPI_INT, parent, ETAPA1, MPI_COMM_WORLD);
		}

	}

	//MPI_Barrier(MPI_COMM_WORLD);

	int cpy_vecini[line];int nr_vecini = 0;
	for(i = 0; i< line; i++){
		if(vecini[i] == tab_rutare[vecini[i]]){
			cpy_vecini[nr_vecini++] = vecini[i];
		}
	}

	line = nr_vecini;
	for(i=0; i< nr_vecini; i++){
		vecini[i] = cpy_vecini[i];
		adj[(rank * numtasks) + vecini[i]] = 1;
		adj[(vecini[i] * numtasks) + rank] = 1;
	}

	//MPI_Barrier(MPI_COMM_WORLD);
	/*se creeaza matricea de adiacenta*/
	if(line == 1 && rank != 0){
		/*este frunza*/
		MPI_Send(adj, numtasks*numtasks, MPI_INT, parent, 10, MPI_COMM_WORLD);
	}

	else{
		int aux_adj[numtasks*numtasks] = {0};
		for(i = 0; i< line -1 ; i++){
			MPI_Recv(aux_adj, numtasks *numtasks, MPI_INT, MPI_ANY_SOURCE, 10, MPI_COMM_WORLD, &Stat);

			int ss = Stat.MPI_SOURCE;

			for(j = 0; j< numtasks * numtasks; j++){
				adj[j] = (adj[j] | aux_adj[j]);

			}
		}

		if(rank == 0){
			MPI_Recv(aux_adj, numtasks *numtasks, MPI_INT, MPI_ANY_SOURCE, 10, MPI_COMM_WORLD, &Stat);

			int ss = Stat.MPI_SOURCE;

			/*se updateaza tabela*/
			for(j = 0; j< numtasks * numtasks; j++){
				adj[j] = (adj[j] | aux_adj[j]);

			}

			FILE* fp = fopen("rank0.txt", "a");

			/*Scrie tabela de rutare*/

			fprintf(fp, "	--------	\nMatricea de adiacenta este:\n");
			for(i = 0; i< numtasks; i++){
				for(j = 0; j< numtasks; j++){
					fprintf(fp, "%d ", adj[i*numtasks + j]);

				}
				fprintf(fp, "\n");
			}
			fclose(fp);

		}

		else{
			MPI_Send(adj, numtasks*numtasks, MPI_INT, parent, 10, MPI_COMM_WORLD);
		}
	}

	//MPI_Barrier(MPI_COMM_WORLD);

	/**************************************ETAPA2************************************************/


	/*se trimite un mesaj de broadcast pentru a anunta primirea de mesaje*/
	int next_hop;
	char bcast_mes[10];
	for(i = 0; i< numtasks; i++){
		if(i != rank){
			next_hop = tab_rutare[i];
			sprintf(bcast_mes, "B %d %d", i, rank);	
			MPI_Send(bcast_mes, 10, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);
		}

	}

	char temp[10];
	int sursa;
	int bcount = 0;	
	MPI_Barrier(MPI_COMM_WORLD);

	/*varibila care reprezinta nr de mesaje de transmis prin nodul curent
	 *sau care au ca destinatie nodul curent; se initializeaza cu numtasks
	 *i = destinatia */
	int count_bcast_msg[numtasks];
	std::fill_n(count_bcast_msg, numtasks, numtasks);
	count_bcast_msg[rank] = numtasks -1;
	bcount = count_bcast_msg[rank];

	/*daca este frunza*/
	if(line == 1){
		/*primeste doar de la celalalte noduri => numtasks-1*/
		std::fill_n(count_bcast_msg, numtasks, 0);
		count_bcast_msg[rank] = numtasks -1;
		bcount = numtasks -1;
	}

	int dest;
	//MPI_Barrier(MPI_COMM_WORLD);

	while(1){

		/*primeste un mesaj de broadcast*/
		MPI_Recv(temp, 10, MPI_CHAR, MPI_ANY_SOURCE, ETAPA2, MPI_COMM_WORLD, &Stat);


		sscanf(temp, "B %d %d", &dest, &sursa);

		/*directia catre destinatie*/
		next_hop = tab_rutare[dest];		

		/*daca nu s-au calculat deja numarul de iteratii 
		 *pentru fiecare destinatie*/
		if(count_bcast_msg[dest] == numtasks){
			for(i = 0; i< numtasks; i++){
				/*s escade din nr total de noduri 
				 * nodurile care ajung la destinatie
				 * prin next_hop*/
				if(tab_rutare[i] == next_hop)
					count_bcast_msg[dest]--;
			}
			count_bcast_msg[dest]--;
			bcount += count_bcast_msg[dest];
		}

		bcount--;

		if(dest != rank){
			/*daca nodul curent nu este destinatia, se trimite mai departe mesajul*/
			MPI_Send(temp, 10, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);

		}
		else{
			/*daca nodul curent este destinatia */

		}

		/*daca s-au primit toate mesajele de broadcast prin nodul curent*/
		if(bcount <= 0){	
			printf("[%d] INITIALIZARE terminata\n", rank);

			break;
		}
	}

	//MPI_Barrier(MPI_COMM_WORLD);

	/*trimiterea mesajelor propriu-zise*/
	int  tosend = message.size(), bcast_torecv = numtasks-1;

	for(vector<pair<int, string> >::iterator it = message.begin(); it != message.end(); ++it){	
		string str = (*it).second;
		char* buff = new char[90];
		buff[str.size()] = 0;
		memcpy(buff, str.c_str(), str.size());

		/*daca e mesaj de broadcast*/
		if((*it).first == -1){
			for(i = 0; i< numtasks; i++){
				if(rank != i){
					char aux[100];
					sprintf(aux, "M %d %d %s", i, rank, buff);
					next_hop = tab_rutare[i];
					printf("[%d] Trimite %s catre %d\n",rank,  aux, next_hop);
					MPI_Send(aux, 100, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);
				}
			}
		}
		else {
			next_hop = tab_rutare[(*it).first];
			char aux[100];
			sprintf(aux, "M %d %d %s", (*it).first, rank, buff);
			MPI_Send(aux, 100, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);
			printf("[%d] Trimite %s catre %d\n", rank, aux, next_hop);
		}		
	}


	/*se trimit mesaje de broadcast pentru sfarsitul conexiunii*/
	for(i = 0; i< numtasks; i++){
		if(i != rank){
			next_hop = tab_rutare[i];
			sprintf(bcast_mes, "B %d %d", i, rank);	
			MPI_Send(bcast_mes, 10, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);
		}

	}

	MPI_Barrier(MPI_COMM_WORLD);
	std::fill_n(count_bcast_msg, numtasks, numtasks);
	count_bcast_msg[rank] = numtasks -1;
	bcount = count_bcast_msg[rank];

	/*daca este frunza*/
	if(line == 1){
		std::fill_n(count_bcast_msg, numtasks, 0);
		count_bcast_msg[rank] = numtasks -1;
		bcount = numtasks -1;
	}

	MPI_Barrier(MPI_COMM_WORLD);


	char recv_m[100];
	while(1){
		/*se primeste mesaj*/
		MPI_Recv(recv_m, 100, MPI_CHAR, MPI_ANY_SOURCE, ETAPA2, MPI_COMM_WORLD, &Stat);


		/*daca este de broadcast*/
		if(recv_m[0] == 'B'){

			sscanf(recv_m, "B %d %d", &dest, &sursa);

			/*directia catre destinatie*/
			next_hop = tab_rutare[dest];		

			/*daca nu s-au calculat deja numarul de iteratii 
			 *pentru fiecare destinatie*/
			if(count_bcast_msg[dest] == numtasks){
				for(i = 0; i< numtasks; i++){
					if(tab_rutare[i] == next_hop)
						count_bcast_msg[dest]--;
				}
				count_bcast_msg[dest]--;
				bcount += count_bcast_msg[dest];
			}

			bcount--;

			/*daca nodul curent este intermediar
			 * se trimite mai departe mesajul*/
			if(dest != rank){

				MPI_Send(recv_m, 100, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);


			}


			if(bcount <= 0){	
				printf("[%d] BROADCAST terminat\n", rank);

				break;
			}
		}
		else {

			char msg[100];
			if(recv_m[0] == 'M'){

				sscanf(recv_m, "M %d %d %[^\n]", &dest, &sursa, msg);


				//directia catre destinatie
				next_hop = tab_rutare[dest];		

				if(dest != rank){

					printf("[%d] Primit \"%s\" prin %d; Se redirectioneaza catre %d\n", rank, msg, Stat.MPI_SOURCE, next_hop);
					MPI_Send(recv_m, 100, MPI_CHAR, next_hop, ETAPA2, MPI_COMM_WORLD);


				}
				else{

					printf("[%d] \"%s\" - mesaj final cu sursa: %d\n", rank, msg, sursa);


				}
			}
		}
	}


	MPI_Barrier(MPI_COMM_WORLD);

	/********************************ETAPA3**************************************************************/
	int pres, vicepres;
	int votes_array[numtasks];
	int votes_aux[numtasks];

	/*vot pentru pres rand*/
	srand(rank * (unsigned) time(0));
	pres = rand() %numtasks;

	std::fill_n(votes_array, numtasks, 0);

	votes_array[pres] = 1;

	/*daca este frunza, se trimite vectorul de voturi*/
	if(line == 1){
		if(rank != 0)
			MPI_Send(votes_array, numtasks, MPI_INT, parent, PTAG, MPI_COMM_WORLD);
	}
	else if(rank != 0){
		for(i = 0; i< line; i++){
			if(vecini[i] != parent){
				MPI_Recv(votes_aux, numtasks, MPI_INT, vecini[i], PTAG, MPI_COMM_WORLD, &Stat);
				int k;
				/*se acumuleaza voturile*/
				for(k = 0; k< numtasks; k++)
					votes_array[k] += votes_aux[k];
			}	
		}
		/*i se trimit voturile parintelui*/
		MPI_Send(votes_array, numtasks, MPI_INT, parent, PTAG, MPI_COMM_WORLD);

	}

	/*daca este nodul 0*/
	if(rank == 0){

		/*primeste de la vecini vectorul de voturi*/
		for(i = 0; i< line; i++){
			MPI_Recv(votes_aux, numtasks, MPI_INT, vecini[i], PTAG, MPI_COMM_WORLD, &Stat);

			int k;
			for(k = 0; k< numtasks; k++)
				votes_array[k] += votes_aux[k];

		}

		/*se afiseaza vectorul de voturi*/
		for(i = 0; i< numtasks; i++)
			printf("VOTES[0] %d %d\n", i, votes_array[i]);
		/*se afla cele mai mari 2 valori pentru presedinte si vicepresedinte*/
		int max1 = -1, max2= -1, p1 = -1, p2 = -1;
		(votes_array[0]> votes_array[1])? (max1 = votes_array[0],p1 = 0) : (max1 = votes_array[1], p1 = 1);

		(p1 == 1)? (max2 = votes_array[0], p2 = 0) : (max2 = votes_array[1], p2 =1); 

		for(i = 2; i< numtasks; i++){
			if(votes_array[i] > max1){
				max2 = max1; p2 = p1;
				max1 = votes_array[i];
				p1 = i;
			}
			else if(votes_array[i] > max2){
				max2 = votes_array[i];
				p2 = i;
			}	
		}

		pres = p1;
		vicepres = p2;
		/*se trimit valorile celor 2 catre restul nodurilor*/	
		for(i = 0; i< line; i++){
			MPI_Send(&pres, 1, MPI_INT, vecini[i], PTAG, MPI_COMM_WORLD);
			MPI_Send(&vicepres, 1, MPI_INT, vecini[i], VTAG, MPI_COMM_WORLD);
		}


	}

	else {
		MPI_Recv(&pres, 1, MPI_INT, parent, PTAG, MPI_COMM_WORLD, &Stat);
		MPI_Recv(&vicepres, 1, MPI_INT, parent, VTAG, MPI_COMM_WORLD, &Stat);

		printf("[%d] Presedinte = %d Vicepresedinte = %d\n", rank, pres, vicepres);
		/*propagarea informatiilor*/
		for(i = 0; i<line; i++){
			if(vecini[i] != parent){
				MPI_Send(&pres, 1, MPI_INT, vecini[i], PTAG, MPI_COMM_WORLD);
				MPI_Send(&vicepres, 1, MPI_INT, vecini[i], VTAG, MPI_COMM_WORLD);


			}
		}

	}




	MPI_Finalize();
	fclose(fin);
	fclose(fmes);

}


