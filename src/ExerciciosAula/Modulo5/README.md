# 🎨 Resposta ao Desafio do Módulo 5

## 📌 Objetivo

O exercício **RespostaAoDesafio** do Módulo 5 foi desenvolvido com o objetivo de implementar uma câmera em primeira pessoa:

### ✅ Funcionalidades implementadas:

- Implementação da câmera FPS como a classe `Camera`
- Movimento da câmera com `W`, `A`, `S` e `D`
- Rotação da câmera com o mouse
- Controle do zoom/FOV com a roda de rolagem
- Controle de objetos ativos na cena com teclado
- `X`, `Y` e `Z`: girar o objeto ativo
- `[` e `]`: escalar o objeto ativo
- `N` e `TAB`: adicionar ou alternar o objeto ativo
- `ESC`: fechar a janela
- Encapsulamento dos atributos da câmera, incluindo posição, direção, velocidade e sensibilidade

---

## 🛠️ Configuração do Ambiente

A configuração do ambiente foi realizada com base no guia:

🔗 https://github.com/FNBergamo/CGCCHibrido/blob/main/GettingStarted.md

Foram utilizadas as dependências necessárias para desenvolvimento com OpenGL:

- OpenGL
- GLFW
- GLAD
- GLM
- stb_image

---

## 📁 Estrutura do Módulo

```text
src/ExerciciosAula/Modulo5/
├── RespostaAoDesafio.cpp    # Código-fonte principal com a classe Camera
└── README.md                # Documentação da atividade
```

---

## ▶️ Como executar

1. Abra o terminal na raiz do repositório e gere o projeto com CMake:

```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

2. Execute o binário gerado para o Módulo 5:

```powershell
.\Modulo5_RespostaAoDesafio.exe
```

> Observação: no Linux/macOS, o binário pode não ter extensão. Nesse caso, execute `./Modulo5_RespostaAoDesafio`.

---

## 🖼️ Resultado

Abaixo está o print da execução do programa:

![Resultado](../../../assets/Screenshots/Modulo5/RespostaAoDesafio/result.mp4)

---

## 📚 Conceitos trabalhados

Durante o desenvolvimento deste desafio foram aplicados os seguintes conceitos:

- Câmera em primeira pessoa
- Vetores de direção da câmera
- Matriz de visão com `glm::lookAt`
- Projeção em perspectiva com `glm::perspective`
- Controle de entrada via teclado e mouse
- Encapsulamento de comportamento em classe
