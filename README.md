# Painel de Cenas para OBS Studio

Plugin nativo que adiciona ao OBS um painel no estilo do vMix: uma grade com **miniaturas ao vivo** de todas as cenas, renderizadas pela placa de vídeo, com marcação de **Prévia** (verde) e **No ar** (vermelho).

## Recursos

- Miniaturas ao vivo de todas as cenas, na ordem da lista do OBS
- Clique: manda a cena para a Prévia · Duplo clique: coloca no ar
- Botões **CORTE** e **TRANSIÇÃO**, com escolha da transição
- Liga/desliga o **Modo de estúdio**
- **Gravar** e **Transmitir** com cronômetro (pedem um segundo clique para confirmar)
- Teclas **1–9** escolhem a cena e **Espaço** faz a transição (com o painel em foco)
- Tamanho das miniaturas ajustável

## Instalação (Windows)

1. Feche o OBS.
2. Baixe o instalador `obs-painel-cenas-*-windows-x64-instalador.exe` em **Actions** (último build) ou em **Releases**.
3. Execute o instalador.
4. Abra o OBS e ative em **Painéis › Painel de Cenas**.

O instalador coloca o plugin em `C:\ProgramData\obs-studio\plugins\obs-painel-cenas`, compatível com o OBS 30 em diante, incluindo a nova estrutura de pastas do OBS 33.

## Compilação

Baseado no [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate). Cada envio para `main` compila automaticamente pelo GitHub Actions.

## Licença

GPL-2.0, como o OBS Studio.
