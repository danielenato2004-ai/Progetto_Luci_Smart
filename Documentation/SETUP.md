# Guida di Setup - Progetto Luci Smart

## Prerequisiti

Installa il seguente software:

1. **STM32CubeMX** (gratuito)
   - Scarica da: https://www.st.com/en/development-tools/stm32cubemx.html
   - Utilizzato per configurare il microcontrollore

2. **STM32CubeIDE** (gratuito)
   - Scarica da: https://www.st.com/en/development-tools/stm32cubeide.html
   - IDE integrato per compilazione e debugging

3. **Git**
   - Scarica da: https://git-scm.com/

## Primo Setup

### 1. Clona il repository
```bash
git clone https://github.com/danielenato2004-ai/Progetto_Luci_Smart.git
cd Progetto_Luci_Smart
```

### 2. Apri la configurazione in STM32CubeMX
```bash
# Naviga alla cartella del progetto
cd STM32_Project

# Apri il file .ioc con STM32CubeMX
# Oppure apri STM32CubeMX e seleziona: File → Open Project → STM32_Project.ioc
```

### 3. Genera il codice
- In STM32CubeMX: `Project → Generate Code`
- Questo genererà tutti i file necessari nella cartella `Core/`

### 4. Apri in STM32CubeIDE
```bash
# STM32CubeIDE aprirà automaticamente la cartella
# Se non accade, seleziona: File → Open Existing Project
```

### 5. Compila il progetto
- Menu: `Project → Build Project`
- Oppure tasto: `Ctrl + B`

### 6. Carica il firmware
- Collega il tuo STM32 Board via USB
- Menu: `Run → Debug As → STM32 MCU C/C++ Application`
- Oppure tasto: `F11`

## Struttura delle Cartelle

```
STM32_Project/
├── Core/
│   ├── Src/              # File sorgente (.c)
│   └── Inc/              # Header file (.h)
├── Drivers/
│   ├── CMSIS/           # ARM Cortex Microcontroller Software Interface Standard
│   └── STM32H7xx_HAL_Driver/  # HAL per il tuo STM32
├── Middlewares/          # Librerie aggiuntive (se necessarie)
├── STM32_Project.ioc    # Configurazione (NON modificare direttamente)
└── STM32H7xx_NUCLEO_H743ZI.ld  # Linker script
```

## Flusso di Sviluppo

### Se modifichi il codice in `Core/Src/`:
```bash
# Compila
# Carica il firmware
# Testa
# Commit e push
git add Core/
git commit -m "Update main.c with new feature"
git push
```

### Se modifichi la configurazione (PIN, clock, etc):
```bash
# Apri STM32CubeMX
# Modifica il file .ioc
# Genera il codice (Project → Generate Code)
# Compila
# Carica il firmware
# Commit il file .ioc
git add STM32_Project/STM32_Project.ioc
git commit -m "Update STM32 configuration"
git push
```

## Troubleshooting

### Problema: "Project not found"
**Soluzione:** Assicurati di avere clonato correttamente il repository con tutti i file.

### Problema: Errore di compilazione
**Soluzione:** 
- Pulisci il progetto: `Project → Clean`
- Rigenerato il codice da STM32CubeMX

### Problema: Firmware non si carica
**Soluzione:**
- Verifica la connessione USB
- Installa i driver ST-Link
- Controlla che il debugger sia correttamente selezionato

## Risorse Utili

- [Documentazione STM32CubeMX](https://www.st.com/resource/en/user_manual/dm00104712-stm32cubemx-user-guide-stmicroelectronics.pdf)
- [STM32 HAL Documentation](https://www.st.com/en/development-tools/stm32cube.html)
- [STM32CubeIDE Getting Started](https://www.st.com/resource/en/user_manual/dm00629856-stm32cubeide-integrated-development-environment-stmicroelectronics.pdf)

---

**Versione:** 1.0  
**Data:** 2026-05-19
