#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0

#define INICIO 0
#define ATUAL 1
#define FINAL 2
#define TAM 256
#define HASH 11
#define BUCKET 2

typedef struct reg{
	int cod;
	char nome[50];
	char seg[50];
	char tipo[30];
} Registro;

Registro tempInsere[6];
int tempRemove[4];
int tempBusca[4];
int tempIndex[3] = {0, 0, 0};

void inserir(Registro registro);
int codificar(int codigo);
void criarHash();
void insereHash(int cod, int endereco, int offset);
void remover();
void buscar();
void carregarArquivos();
void recuperarIndexes();
void obterCache();
void salvarCache();
void dumpArquivo();

int main(void){

    DIR *temp = opendir("temp");

	if (temp)
		closedir(temp);
	else
		mkdir("temp", 0777);

    obterCache();
    criarHash();
    
    printf("///////////////  SISTEMA DE REGISTRO DE SEGURADORAS  ///////////////\n");
	printf("///////////////                MENU                  ///////////////\n");
	printf("///////////////  1. INSERIR                          ///////////////\n");
	printf("///////////////  2. REMOVER                          ///////////////\n");
	printf("///////////////  3. BUSCAR                           ///////////////\n");
	printf("///////////////  4. CARREGAR ARQUIVOS                ///////////////\n");
    printf("///////////////  5. DUMP DE ARQUIVOS                 ///////////////\n");
	printf("/////////////// -1. SAIR                             ///////////////\n");

    int escolha;

    do{
        printf("\n$ ");
        scanf("%d", &escolha);
        switch(escolha){
            case 1:
                inserir(tempInsere[tempIndex[0]]);
                tempIndex[0]++;
                break;
            case 2:
                remover();
                tempIndex[1]++;
                break;
            case 3:
                buscar();
                tempIndex[2]++;
                break;
            case 4:
                carregarArquivos();
                break;
            case 5:
                dumpArquivo();
                break;
            case -1:
                break;
            default:
                printf("Opcao invalida.\n");
        }
        if(tempIndex[0] >= 6){
            tempIndex[0] = 0;
            printf("Todos os registros foram adicionados, insercao reiniciada.\n");
        }
        if(tempIndex[1] >= 4){
            tempIndex[1] = 0;
            printf("Todos os codigos foram removidos, remocao reiniciada.\n");
        }
        if(tempIndex[2] >= 4){
            tempIndex[2] = 0;
            printf("Todos os codigos foram buscados, busca reiniciada.\n");
        }
        salvarCache();         
    }while(escolha != -1);
}

void criarHash(){
    FILE *hash;

    hash = fopen("./temp/hash.bin", "rb");

    if(hash == NULL){
        hash = fopen("./temp/hash.bin", "w+b");
        
        int i;
        for(i=0; i < HASH*BUCKET; i++){
            int aux = -1;
            fwrite(&aux, sizeof(int), 1, hash);
        }
        fclose(hash);
        printf("Hash criado!\n");
    }
}

void inserir(Registro registro){

	char buffer[sizeof(Registro)];
    
	sprintf(buffer, "%d#%s#%s#%s", registro.cod, registro.nome, registro.seg, registro.tipo);
	int tamanhoRegistro = strlen(buffer);
    
	FILE *data;

	data = fopen("./temp/data.bin", "r+b");

	if (data == NULL)
	{
		printf("Arquivo data.bin criado!\n");
		data = fopen("./temp/data.bin", "w+b");
	}
	else
		fseek(data, 0, FINAL);

	int posicaoData = ftell(data);

	fwrite(&tamanhoRegistro, sizeof(int), 1, data);
    fwrite(&buffer, sizeof(char), tamanhoRegistro, data);
    fclose(data);

    int endereco = codificar(registro.cod);
    
    insereHash(registro.cod ,endereco, posicaoData);
}

int codificar(int codigo){
    return codigo % HASH;
}

void insereHash(int cod, int endereco, int offset){
    //Fazer busca antes p/ verificar se ja existe um codigo igual
    printf("Codigo %d", cod);
    printf("Endereco %d\n", endereco);

    FILE *hash;

    hash = fopen("./temp/hash.bin", "wb");

    fseek(hash, endereco * sizeof(int) * 4, INICIO);
    //Verificar se o arquivo esta cheio, rodando ele inteiro

    int pos1, pos2;
    int contColisao = 0;

    do{
        fread(&pos1, sizeof(int), 1, hash);
        fseek(hash, sizeof(int), ATUAL);
        fread(&pos2, sizeof(int), 1, hash);
        fseek(hash, -sizeof(int), ATUAL);
        fseek(hash, -sizeof(int), ATUAL);
        printf("%d", ftell(hash));

        printf("%d", pos1);
        printf("%d", pos2);

        if(pos1 == -1 || pos1 == -2){
            fwrite(&cod, sizeof(int), 1, hash);
            fwrite(&offset, sizeof(int), 1, hash);
            printf("Codigo %d inserido com sucesso!\n");
            break;
        }else if(pos2 == -1 || pos2 == -2){
            fseek(hash, 2*sizeof(int), ATUAL);
            fwrite(&cod, sizeof(int), 1, hash);
            fwrite(&offset, sizeof(int), 1, hash);
            printf("Codigo %d inserido com sucesso!\n");
            break;
        }else{
            printf("Colisao\n");
            contColisao++;
            printf("Tentativa %d\n", contColisao);
            fseek(hash, 4*sizeof(int), ATUAL);
            if(endereco * sizeof(int) * 4 == ftell(hash)){
                printf("Nao possui espaco no hash!");
                break;
            }

            if(feof(hash))
                fseek(hash, 0, INICIO);
        }
    } while(1);
    fclose(hash);
}

void remover(){
    /*Dado um Código o programa remove o respectivo código do índice hash. 
    Não é necessário realizar a remoção no arquivo principal (o que contém os registros, somente no índice). 
    Para tanto, utilize o processo de remoção associado ao Overflow Progressivo. 
    */
}

void buscar(){
    /*Dado um Código o programa retorna os dados do respectivo Segurado. 
    Para tanto, a busca deve ser feita na hash. 
    Além disso, as seguintes mensagens deverão ser exibidas em relação à busca: 
    1. “Chave X encontrada, endereço E, N acessos” indica que a chave X foi encontrada no endereço E 
    e que foram necessários N acessos para recuperar a informação na hash. 
    Após a exibição dessa mensagem, os dados referentes ao Segurado deverão ser recuperados 
    do arquivo principal; 
    2. “Chave X não encontrada” indica que a Chave X não está presente na hash e, 
    consequente, no arquivo principal. 
    */
}

void carregarArquivos(){
    FILE *insere;

	insere = fopen("./temp-testes/insere.bin", "r+b");
	fread(&tempInsere, sizeof(struct reg), 6, insere);
	fclose(insere);

    FILE *remove;

	remove = fopen("./temp-testes/remove.bin", "r+b");
	fread(&tempRemove, sizeof(int), 4, remove);
	fclose(remove);

    FILE *busca;

	busca = fopen("./temp-testes/busca.bin", "r+b");
	fread(&tempBusca, sizeof(int), 4, busca);
	fclose(busca);

	printf("Dados carregados com sucesso!\n");
}

void obterCache(){
    FILE *cache;

    cache = fopen("./temp/cache.bin", "r+b");

    if(cache == NULL){
        printf("Cache criado!\n");
        salvarCache();
    }else{
        fread(&tempIndex, sizeof(int), 3, cache);
        fclose(cache);
        printf("Cache obtido [%d, %d, %d]\n", tempIndex[0], tempIndex[1], tempIndex[2]);
    }
}

void salvarCache(){
    FILE *cache;

    cache = fopen("./temp/cache.bin", "w+b");
    fwrite(&tempIndex, sizeof(int), 3, cache);
    fclose(cache);
}

void hexDump(size_t offset, void *addr, int len){

	int i;
	unsigned char bufferLine[17];
	unsigned char *pc = (unsigned char *)addr;

	for (i = 0; i < len; i++){
		if ((i % 16) == 0){
			if (i != 0)
				printf(" %s\n", bufferLine);
			printf("%08zx: ", offset);
			offset += (i % 16 == 0) ? 16 : i % 16;
		}
		printf("%02x ", pc[i]);
		if ((pc[i] < 0x20) || (pc[i] > 0x7e)){
			bufferLine[i % 16] = '.';
		}else{
			bufferLine[i % 16] = pc[i];
		}
		bufferLine[(i % 16) + 1] = '\0';
	}

	while ((i % 16) != 0){
		printf("  ");
		if (i % 2 == 1)
			putchar(' ');
		i++;
	}
	printf("  %s\n", bufferLine);
}

void dumpArquivo(){
    FILE *myfile;
    int escolha;

    printf("Escolha o arquivo para fazer o dump:\n\n");

    if (fopen("./temp/data.bin", "rb") != NULL)
        printf(" 1. data.bin\n");
    if (fopen("./temp/cache.bin", "rb") != NULL)
        printf(" 2. cache.bin\n");
    if (fopen("./temp/hash.bin", "rb") != NULL)
        printf(" 3. hash.bin\n");

    printf("-1. RETORNAR\n\n");

    printf("$ ");

    scanf("%d", &escolha);

    if(escolha == -1)
        return;

    switch (escolha){
        case 1:
            myfile = fopen("./temp/data.bin", "rb");
            break;
        case 2:
            myfile = fopen("./temp/cache.bin", "rb");
            break;
        case 3:
            myfile = fopen("./temp/hash.bin", "rb");
            break;
        default:
            printf("Escolha invalida, abortando dump.\n");
            return;
    }

    printf("\n");

    unsigned char buffer[TAM];
    size_t n;
    size_t offset = 0;

    while ((n = fread(buffer, 1, TAM, myfile)) > 0){
        hexDump(offset, buffer, n);
        if (n < TAM)
            break;
        offset += n;
    }
    fclose(myfile);
}