# Progetto Luci Smart

Progetto di elettronica basato su **STM32** per la gestione e risparmio intelligente di sistemi di illuminazione.

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
