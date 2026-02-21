#include "../include/kernel.h"

framebuffer_t fb;

// Prosta funkcja opóźniająca dla efektu wizualnego
static void boot_delay(u32 count) {
    for (volatile u32 i = 0; i < count * 10000; i++) {
        __asm__ volatile("nop");
    }
}

void kernel_main(u32 magic, multiboot_info_t *mbi) {
    // 1. Inicjalizacja sprzętowa (GDT, IDT itp.)
    gdt_init();
    idt_init();
    pic_init();

    // 2. Detekcja Framebuffera
    if (magic == MBOOT2_MAGIC) {
        fb_init_mb2((mb2_info_t *)mbi);
    } else {
        fb_init(mbi);
    }

    // 3. Sekwencja Splash Screen (ArcticOS Boot)
    fb_clear(0x00050A0F); // Ciemny arktyczny granat

    int lx = (fb.width - LOGO_WIDTH) / 2;
    int ly = (fb.height / 2) - (LOGO_HEIGHT / 2) - 20;
    
    // Rysuj logo w kolorze lodowym błękicie
    fb_draw_logo(lx, ly, 0x00A0EFFF); 
    fb_draw_string(lx + 10, ly + LOGO_HEIGHT + 10, "ArcticOS Kernel", 0x00FFFFFF, 0x00000000, 1);

    // 4. Animacja paska ładowania z komunikatami
    int barW = 200;
    int barX = (fb.width - barW) / 2;
    int barY = ly + LOGO_HEIGHT + 40;

    const char* stages[] = {
        "Loading GDT...",
        "Setting up IDT...",
        "Initializing PIC...",
        "Detecting Hardware...",
        "Starting ArcticOS..."
    };

    for (int i = 0; i <= 100; i++) {
        fb_draw_loading_bar(barX, barY, barW, 8, i, 0x0000DFFF);
        
        // Zmiana tekstu co 25% postępu
        if (i % 25 == 0 && (i/25) < 5) {
            // Czyścimy poprzedni tekst małym prostokątem (opcjonalnie)
            fb_fill_rect(barX, barY + 15, barW, 10, 0x00050A0F);
            fb_draw_string(barX, barY + 15, stages[i/25], 0x00AAAAAA, 0x00000000, 1);
        }
        
        boot_delay(150); // Kontrola prędkości animacji
    }

    // 5. Inicjalizacja sterowników po animacji
    timer_init(100);
    keyboard_init();
    rtc_init();
    enable_interrupts();

    // 6. Uruchomienie pulpitu (Desktop)
    boot_delay(500); // Chwila pauzy na 100%
    desktop_init();
    desktop_run();

    for (;;) { __asm__ volatile("hlt"); }
}