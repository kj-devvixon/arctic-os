# ArcticOS - Przewodnik Instalacji i Uruchomienia
## System: Pop!_OS (Ubuntu-based)

---

## KROK 1: Instalacja Wymaganych Narzędzi

Otwórz terminal i wpisz:

```bash
# Aktualizacja systemu
sudo apt update && sudo apt upgrade -y

# Kompilator cross-compile dla x86 32-bit (NIEZBĘDNE)
sudo apt install -y \
    nasm \
    gcc \
    gcc-multilib \
    binutils \
    make \
    grub-pc-bin \
    grub-common \
    xorriso \
    mtools \
    qemu-system-x86 \
    qemu-kvm \
    git

# Sprawdź czy wszystko się zainstalowało:
nasm --version
qemu-system-i386 --version
grub-mkrescue --version
```

---

## KROK 2: Opcjonalnie - Cross-Compiler i686-elf-gcc

Zwykły `gcc` **może** nie działać poprawnie dla kernela.
Dla najlepszych wyników zainstaluj cross-compiler:

```bash
# Metoda 1: Szybka (gotowy pakiet)
sudo apt install -y gcc-i686-linux-gnu

# Jeśli pakiet dostępny, edytuj Makefile:
# Zmień: CROSS_CC := i686-elf-gcc
# Na:    CROSS_CC := i686-linux-gnu-gcc

# Metoda 2: Ze źródeł (wolniejsza ale czysta)
# Skrypt do pobrania i zbudowania cross-compilera:

cat > build_cross.sh << 'SCRIPT'
#!/bin/bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p ~/src && cd ~/src

# Pobierz binutils
wget https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.gz
tar xf binutils-2.40.tar.gz
mkdir build-binutils && cd build-binutils
../binutils-2.40/configure --target=$TARGET --prefix=$PREFIX \
    --with-sysroot --disable-nls --disable-werror
make -j$(nproc) && make install
cd ..

# Pobierz GCC
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
tar xf gcc-13.2.0.tar.gz
mkdir build-gcc && cd build-gcc
../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX \
    --disable-nls --enable-languages=c --without-headers
make -j$(nproc) all-gcc && make install-gcc
make -j$(nproc) all-target-libgcc && make install-target-libgcc

echo "Cross-compiler gotowy w: $PREFIX/bin"
echo "Dodaj do PATH: export PATH=\$HOME/opt/cross/bin:\$PATH"
SCRIPT

chmod +x build_cross.sh
# ./build_cross.sh  # Odkomentuj żeby uruchomić (zajmuje ~20 min)
```

---

## KROK 3: Kompilacja ArcticOS

```bash
# Wejdź do folderu projektu
cd ~/arcticos    # lub gdzie umieściłeś projekt

# Sprawdź strukturę plików:
ls -la
# Powinno pokazać: Makefile, boot/, kernel/, drivers/, apps/, libc/, include/, grub/

# KOMPILACJA KERNELA:
make

# Jeśli błąd z kompilatorem, spróbuj:
make CC=gcc CROSS=0

# Stworzenie ISO:
make iso

# Powinieneś zobaczyć: arcticos.iso
ls -lh arcticos.iso
```

---

## KROK 4: Uruchomienie w QEMU

```bash
# Metoda 1: Jedną komendą (kompiluj + ISO + uruchom)
make run

# Metoda 2: Ręcznie
qemu-system-i386 \
    -cdrom arcticos.iso \
    -m 128M \
    -vga std \
    -no-reboot

# Metoda 3: Z więcej opcji QEMU
qemu-system-i386 \
    -cdrom arcticos.iso \
    -m 256M \
    -vga vmware \
    -display gtk,zoom-to-fit=on \
    -no-reboot \
    -no-shutdown
```

---

## KROK 5: Sterowanie w ArcticOS

Po uruchomieniu zobaczysz pulpit ArcticOS:

| Klawisz | Akcja |
|---------|-------|
| `1` | Otwórz aplikację Zegar (RTC) |
| `2` | Otwórz Terminal/Shell |
| `3` | Otwórz Edytor Tekstu |
| `ESC` | Wróć do pulpitu (z aplikacji) |

### W Terminalu:
| Komenda | Opis |
|---------|------|
| `help` | Lista komend |
| `time` | Aktualny czas z RTC |
| `uname` | Info o systemie |
| `cpuid` | Info o procesorze |
| `uptime` | Czas działania |
| `meminfo` | Info o framebuffer |
| `echo tekst` | Wypisz tekst |
| `color` | Test kolorów |
| `clear` | Wyczyść ekran |
| `exit` | Wróć do pulpitu |

### W Zegarze:
- Analogowa + cyfrowa tarcza
- Dane z prawdziwego CMOS RTC
- `ESC` lub `q` - wyjście

### W Edytorze:
- Pisz normalnie
- `BACKSPACE` - usuń znak
- `ENTER` - nowa linia
- `Ctrl+A` - początek linii
- `Ctrl+E` - koniec linii
- `ESC` - wyjście

---

## Rozwiązywanie Problemów

### Problem: "grub-mkrescue: command not found"
```bash
sudo apt install grub-pc-bin xorriso
# lub
sudo apt install grub2-common
```

### Problem: Błąd linkowania 32-bit
```bash
sudo apt install gcc-multilib libc6-dev-i386
```

### Problem: QEMU nie otwiera okna
```bash
# Zainstaluj GTK backend
sudo apt install qemu-system-x86 libgtk-3-0

# Lub użyj SDL
qemu-system-i386 -cdrom arcticos.iso -m 128M -display sdl
```

### Problem: "undefined reference to..."
```bash
# Upewnij się że wszystkie pliki C są w Makefile
# Sprawdź sekcję C_SOURCES w Makefile
```

### Uruchomienie na prawdziwym sprzęcie (USB)
```bash
# UWAGA: Nadpisze dane na USB!
sudo dd if=arcticos.iso of=/dev/sdX bs=4M status=progress
# Gdzie /dev/sdX to twój pendrive (sprawdź przez: lsblk)
```

---

## Struktura Projektu

```
arcticos/
├── Makefile              # System budowania
├── linker.ld             # Skrypt linkera
├── boot/
│   └── boot.asm          # Bootloader (ASM, Multiboot)
├── kernel/
│   ├── kernel.c          # Punkt wejścia kernela
│   ├── gdt.c             # Global Descriptor Table
│   ├── idt.c             # Interrupt Descriptor Table + PIC
│   └── desktop.c         # Menedżer pulpitu
├── drivers/
│   ├── framebuffer.c     # VESA VBE + czcionka 8x16 + grafika
│   ├── keyboard.c        # Sterownik PS/2 + US QWERTY
│   ├── rtc.c             # Real Time Clock (CMOS)
│   └── timer.c           # PIT 8253 (100Hz)
├── apps/
│   ├── clock.c           # Zegar analogowy + cyfrowy
│   ├── terminal.c        # Shell z komendami
│   └── editor.c          # Edytor tekstu
├── libc/
│   └── libc.c            # Biblioteka C (bez stdlib)
├── include/
│   └── kernel.h          # Nagłówki, typy, deklaracje
└── grub/
    └── grub.cfg          # Konfiguracja GRUB
```

---

## Techniczne Szczegóły

- **Architektura:** i386/i686, 32-bit Protected Mode
- **Boot:** GRUB2 Multiboot 1
- **Wyświetlanie:** VESA VBE przez GRUB (800x600 lub wyżej)
  - Działa na: VGA, HDMI, DisplayPort, DVI przez BIOS/UEFI
- **Czcionka:** Wbudowana 8x16 pikseli (ASCII 32-127)
- **Klawiatura:** PS/2, US QWERTY layout
- **Timer:** PIT 8253, 100Hz
- **RTC:** CMOS 0x70/0x71, BCD + bin
- **Pamięć:** Flat memory model, brak MMU/paging
- **Przerwania:** IDT, PIC 8259A, IRQ 0,1,8
