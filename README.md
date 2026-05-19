# Progetto Luci Smart

Progetto di elettronica basato su **STM32** per la gestione intelligente di sistemi di illuminazione.

## 📋 Descrizione

Questo repository contiene il firmware e la configurazione per un microcontrollore STM32 dedicato al controllo di luci smart.

## 🛠️ Requisiti

- **STM32CubeMX** - IDE per la configurazione STM32
- **STM32CubeIDE** - IDE per la compilazione e il debugging
- **STM32 Microcontroller** - Hardware di destinazione

## 📁 Struttura del Progetto

```
.
├── STM32_Project/          # Progetto STM32CubeMX
│   ├── Core/               # Codice sorgente principale
│   ├── Drivers/            # HAL e driver STM32
│   └── STM32_Project.ioc   # Configurazione CubeMX
├── Documentation/          # Documentazione
├── README.md              # Questo file
└── .gitignore            # File da escludere da Git
```

## 🚀 Come Iniziare

1. **Clona il repository**
   ```bash
   git clone https://github.com/danielenato2004-ai/Progetto_Luci_Smart.git
   cd Progetto_Luci_Smart
   ```

2. **Apri il progetto in STM32CubeMX**
   - Apri `STM32_Project/STM32_Project.ioc`
   - Genera il codice se necessario

3. **Compila in STM32CubeIDE**
   - Apri il progetto in STM32CubeIDE
   - Build e Debug

## 📚 Documentazione

Vedi la cartella `Documentation/` per:
- **SETUP.md** - Guida di configurazione
- **ARCHITECTURE.md** - Architettura del progetto
- **USAGE.md** - Guida d'utilizzo

## 📝 Note

- I file compilati (`.elf`, `.hex`, `.bin`) **non** sono tracciati da Git
- Ogni sviluppatore compila localmente il proprio firmware
- Il file `.ioc` è la fonte di verità per la configurazione

## 📧 Contatti

Creato da: danielenato2004-ai

---

**Ultimo aggiornamento:** 2026-05-19
