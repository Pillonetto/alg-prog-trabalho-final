#include <stdio.h>
#include <ctype.h>
#include "raylib.h"
#include "render_player.h"
#include "render_jogo.h"
#include "jogo.h"
#include "caixas.h"
#include "fundo.h"
#include "menu_principal.h"

#define TAM_BARRA 10

// Macro para a escala dos objetos na tela
#define SCALE Scale(render.texture.height)

void inicializaPlayer(Player *player, Mapa mapa){

    player->render = (Rectangle){.width = TAM_TILES,
                            .height = TAM_TILES,
                            .x = (player->x * TAM_TILES),
                            .y = (player->y * TAM_TILES)};
    player->x = 1;
    player->y = 8;
    player->vidas = 3;
    player->pontos = 0;
    player->chave = 0;
    player->estado = IDLE;
    player->fase = 1;
    player->quedaDano = 0;
    player->spriteAtual.x = 0;

    player->render.x = player->x * TAM_TILES;
    player->render.y = player->y * TAM_TILES;

}

void passaFase(Player *player, Mapa mapa, int *caixasTotal, int *caixasAbertas){
    player->fase++;
    if (player->vidas < 3)
        player->vidas++;
    player->x = 1;
    player->y = mapa.linhas - 2;
    player->render.x = player->x * TAM_TILES;
    player->render.y = player->y * TAM_TILES;
    (*caixasTotal)++;
    *caixasAbertas = 0;

}

void Jogo(Mapa *mapa, Texture2D tileset, Player *player, int frames, AnimacaoArr *caixa, int *caixasAbertas,
          int caixas[MAX_CAIXAS], AnimacaoItem *explosao, Vector2 *renderPos, AnimacaoItem itens[N_ITENS],
          int *telaAtual) {

    if (player->estado == IDLE && itens[1].flag == 0 && *telaAtual == JOGO)
    {
        // fun��es de movimenta��o aqui:
        if (IsKeyPressed(KEY_LEFT))
            MovimentoHorizontal(mapa, player, -1);
        if (IsKeyPressed(KEY_RIGHT))
            MovimentoHorizontal(mapa, player, +1);
        if (IsKeyPressed(KEY_UP))
            MovimentoVertical(mapa, player, -1, caixasAbertas, caixas, itens);
        if (IsKeyPressed(KEY_DOWN))
            MovimentoVertical(mapa, player, +1, caixasAbertas, caixas, itens);
        if (IsKeyPressed(KEY_S))
            *telaAtual = SAVE;
    }

    AnimaPlayerPos(player, mapa->matriz);
    RenderJogo(*mapa, tileset, player, frames, caixa, explosao, renderPos, itens, telaAtual);
}

/* Controle de movimento vertical
   @param *mapa Utilizado para atualizar posicoes na matriz
   @param *player Estrutura contem localizacao atual do jogador
   @param direcao Movimento desejado pelo jogador (-1 para cima ou +1 para baixo)
   @param *caixasAbertas Numero de caixas ja abertas nessa fase. Utilizado para controle de caixas
   @param caixas Vetor com os itens desta fase
   @param itens Vetor que controle animacao ao receber cada item */

void MovimentoVertical(Mapa *mapa, Player *player, int direcao, int *caixasAbertas, int caixas[MAX_CAIXAS], AnimacaoItem itens[N_ITENS]){
    //Localizacao do player e portas na matriz mapa
    char posAtualPlayer = mapa->matriz[player->y][player->x];
    int portaX, portaY, item;

    //Player esta em porta?
    if (isdigit(posAtualPlayer)){
        busca_porta(*mapa, player->y, player->x, &portaY, &portaX);

        player->x = portaX;
        player->y = portaY;
        player->render.x = portaX*24;
        player->render.y = portaY*24;

        return;

    }

    //Caso player esteja interagindo com caixa
    if(posAtualPlayer == 'C'){

        //Recebe o valor do item da caixa aberta
        item = abreCaixa(*caixasAbertas, caixas, mapa, player);

        //Aumenta a contagem de caixas abertas
        ++(*caixasAbertas);

        //Para testes
        printf("ITEM RECEBIDO %d\n", item);

        //Interpretacao do item recebido
        switch(item){

                case CHAVE:
                    //Som?
                    player->chave = 1;
                    mapa->matriz[mapa->fim_y][mapa->fim_x] = 'P';
                    itens[0].flag = 1;
                    break;
                case BOMBA:
                    itens[1].flag = 1;
                    break;
                default:
                    if (item == 50) itens[6].flag = 1;
                    else if (item == 100) itens[5].flag = 1;
                    else if (item == 150) itens[4].flag = 1;
                    else if (item == 200) itens[3].flag = 1;
                    else if (item == 300) itens[2].flag = 1;
                    player->pontos += item;
                    break;

        }

        return;

    }

    if (posAtualPlayer == 'P' && player->chave == 1){

        //Fim de fase atual.
        mapa->fim = 1;

        return;
    }

    //Impede o jogador de andar por paredes
    if (mapa->matriz[player->y + direcao][player->x] == 'X')
        return;

    //Se player estiver em escada ou em cima de escada, atualiza o valor X conforme o movimento desejado
    if (posAtualPlayer == 'H' || mapa->matriz[player->y+direcao][player->x] == 'H'){

        //ESTADO->ESCADA
        player->y += direcao;

    }
    //Se nao estiver em escada, movimento vertical nao e permitido

}

/* Controle de movimento horizontal
   @param *mapa Contem a matriz do jogo
   @param *player Contem posicao atual do jogador
   @param direcao Movimento desejado pelo jogador (-1 para esquerda e +1 para direta) */

void MovimentoHorizontal(Mapa *mapa, Player *player, int direcao){

    int colunas = mapa->colunas;
    //Variavel de contagem da altura da queda
    int blocosQueda = 0;

    //Impede o jogador de sair dos limites do mapa
    if (player->x + direcao > colunas || player->x + direcao < 0)
        return;

    //Impede o jogador de andar sobre paredes
    if (mapa->matriz[player->y][player->x + direcao] == 'X')
        return;

    //ESTADO->ANDANDO

    //Atualiza o valor Y conforme movimento desejado
    player->x += direcao;

    //Char da matriz abaixo da posicao atual do player
    char posAbaixo = mapa->matriz[player->y + 1][player->x];

    //Player andou para posicao sem chao?

    //Enquanto jogador nao estiver sobre escada ou chao
    while (posAbaixo != 'H' && posAbaixo != 'X'){

        //ESTADO->CAINDO

        player->y++;
        blocosQueda++;

        //Para continuar realizando teste da posicao abaixo
        posAbaixo = mapa->matriz[player->y + 1][player->x];

    }

    //Queda de 3 blocos ou mais reduz vida.
    if (blocosQueda > 2) {
        player->quedaDano = 1;
    }

}

/* Funcao Auxilia no movimento entre portas
   @param mapa Contem matriz do jogo
   @param playerX posicao linha do jogador
   @param playerY posicao coluna do jogador
   @param *x_porta recebe posicao linha da porta encontrada
   @param *y_porta recebe posicao coluna da porta encontrada */

void busca_porta(Mapa mapa, int playerX, int playerY, int *x_porta, int *y_porta) {

    int linhas = mapa.linhas;
    int colunas = mapa.colunas;

 /* Como a porta em que se localiza o player nao eh apagada da matriz, os testes
    buscam porta de mesmo numero, mas de posicao diferente do parametro passado */


    for (int i = 0; i < linhas; i++){

        for (int j = 0; j < colunas; j++){

            if ( mapa.matriz[i][j] == mapa.matriz[playerX][playerY]
                && (i != playerX || j != playerY) ) {

                *x_porta = i;
                *y_porta = j;

            }

        }

    }
}
