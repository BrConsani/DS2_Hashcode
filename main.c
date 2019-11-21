#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0
#define NULO -1
#define VAGO 0xfefefefe

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
void remover(int codigo);
void buscar(int codigo);
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
                remover(tempRemove[tempIndex[1]]);
                tempIndex[1]++;
                break;
            case 3:
                buscar(tempBusca[tempIndex[2]]);
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

    hash = fopen("./temp/hash.bin", "r+b");

    if(hash == NULL){
        hash = fopen("./temp/hash.bin", "wb");
        int i;
        for(i=0; i < HASH*2*BUCKET; i++){
            int aux = NULO;
            fwrite(&aux, sizeof(int), 1, hash);
        }
        printf("Hash criado!\n");
    }
    fclose(hash);
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
		data = fopen("./temp/data.bin", "wb");
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
    printf("Codigo %d\n", cod);
    printf("Endereco %d\n", endereco);

    FILE *hash;

    hash = fopen("./temp/hash.bin", "r+b");

    fseek(hash, endereco * sizeof(int) * 4, INICIO);

    int bucket[4];
    int contColisao = 0;

    do{
        fread(&bucket, sizeof(int), 4, hash);
        fseek(hash, -4*sizeof(int), ATUAL);

        if(bucket[0] == NULO || bucket[0] == VAGO){
            fwrite(&cod, sizeof(int), 1, hash);
            fwrite(&offset, sizeof(int), 1, hash);
            printf("Codigo %d inserido com sucesso!\n", cod);
            break;
        }else if(bucket[2] == NULO || bucket[2] == VAGO){
            fseek(hash, 2*sizeof(int), ATUAL);
            fwrite(&cod, sizeof(int), 1, hash);
            fwrite(&offset, sizeof(int), 1, hash);
            printf("Codigo %d inserido com sucesso!\n", cod);
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

void remover(int codigo){

    int endereco = codificar(codigo);

    FILE *hash;

    hash = fopen("./temp/hash.bin", "r+b");
    fseek(hash, endereco * sizeof(int) * 4, INICIO);

    int info;

    do{
        fread(&info, sizeof(int), 1, hash);
        fseek(hash, sizeof(int), ATUAL);
    }while(info != codigo && info != NULO);

    if(info == NULO){
        printf("O codigo %d nao existe!\n", codigo);
        return;
    }

    fseek(hash, -2*sizeof(int), ATUAL);

    int aux = VAGO;
    fwrite(&aux, sizeof(int), 1, hash);
    aux = NULO;
    fwrite(&aux, sizeof(int), 1, hash);

    fclose(hash);
    printf("Codigo %d removido com sucesso!\n", codigo);
}

void buscar(int codigo){
    FILE *hash;
    hash = fopen("./temp/hash.bin", "rb");
    int endereco = codificar(codigo);
    fseek(hash, endereco * sizeof(int) * 4, INICIO);
    
    int info;
    int acessos = 0;
    do{
        fread(&info, sizeof(int), 1, hash);
        fseek(hash, sizeof(int), ATUAL);
        acessos++;
    }while(info != codigo && info != NULO);
    
    if(info == NULO){
        printf("Codigo %d nao encontrado!\n", codigo);
        return;
    }

    int enderecoPrincipal;
    fseek(hash, -sizeof(int), ATUAL);
    fread(&enderecoPrincipal, sizeof(int), 1, hash);

    fclose(hash);
    
    FILE *data;
    data = fopen("./temp/data.bin", "rb");
    fseek(data, enderecoPrincipal, INICIO);
    
    int tamanhoRegistro;
    fread(&tamanhoRegistro, sizeof(int), 1, data);
    
    char registro[tamanhoRegistro];
    fread(&registro, sizeof(char), tamanhoRegistro, data);

    fclose(data);

    printf("Codigo %d encontrado, endereco %d, %d acessos.\n", codigo, endereco, acessos);
    printf("Dados: %s\n", registro);
}

void carregarArquivos(){
    FILE *insere;

	insere = fopen("./temp-testes/insere.bin", "rb");
	fread(&tempInsere, sizeof(struct reg), 6, insere);
	fclose(insere);

    FILE *remove;

	remove = fopen("./temp-testes/remove.bin", "rb");
	fread(&tempRemove, sizeof(int), 4, remove);
	fclose(remove);

    FILE *busca;

	busca = fopen("./temp-testes/busca.bin", "rb");
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