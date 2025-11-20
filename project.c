/* portfolio.c
   Simple Investment Portfolio Management System in C
   Compile: gcc portfolio.c -o portfolio
   Run: ./portfolio
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 64
#define TYPE_LEN 32
#define FILE_NAME "portfolio.csv"

typedef struct {
    int id;
    char name[NAME_LEN];
    char type[TYPE_LEN]; /* e.g., Stock, Bond, ETF, Crypto */
    double quantity;
    double price; /* current price per unit */
} Asset;

typedef struct {
    Asset *items;
    int count;
    int next_id;
} Portfolio;

/* --- Utility --- */
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

double read_double(const char *prompt) {
    double x;
    int rc;
    while (1) {
        printf("%s", prompt);
        rc = scanf("%lf", &x);
        if (rc == 1) { clear_input_buffer(); return x; }
        printf("Invalid number. Try again.\n");
        clear_input_buffer();
    }
}

int read_int(const char *prompt) {
    int x;
    int rc;
    while (1) {
        printf("%s", prompt);
        rc = scanf("%d", &x);
        if (rc == 1) { clear_input_buffer(); return x; }
        printf("Invalid integer. Try again.\n");
        clear_input_buffer();
    }
}

void read_string(const char *prompt, char *out, int len) {
    printf("%s", prompt);
    if (fgets(out, len, stdin) == NULL) {
        out[0] = '\0';
        return;
    }
    /* strip newline */
    size_t L = strlen(out);
    if (L > 0 && out[L-1] == '\n') out[L-1] = '\0';
}

/* --- Portfolio management --- */
void init_portfolio(Portfolio *p) {
    p->items = NULL;
    p->count = 0;
    p->next_id = 1;
}

void free_portfolio(Portfolio *p) {
    free(p->items);
    p->items = NULL;
    p->count = 0;
}

Asset *find_asset_by_id(Portfolio *p, int id) {
    for (int i = 0; i < p->count; ++i) {
        if (p->items[i].id == id) return &p->items[i];
    }
    return NULL;
}

void add_asset(Portfolio *p) {
    Asset a;
    a.id = p->next_id++;

    read_string("Asset name: ", a.name, NAME_LEN);
    read_string("Asset type (Stock/Bond/ETF/Crypto/...): ", a.type, TYPE_LEN);
    a.quantity = read_double("Initial quantity: ");
    a.price = read_double("Initial price per unit: ");

    Asset *tmp = realloc(p->items, sizeof(Asset) * (p->count + 1));
    if (!tmp) {
        printf("Memory allocation failed. Asset not added.\n");
        return;
    }
    p->items = tmp;
    p->items[p->count++] = a;
    printf("Added asset ID %d: %s (%s)\n", a.id, a.name, a.type);
}

void remove_asset(Portfolio *p) {
    int id = read_int("Enter asset ID to remove: ");
    int idx = -1;
    for (int i = 0; i < p->count; ++i) if (p->items[i].id == id) { idx = i; break; }
    if (idx == -1) { printf("Asset ID %d not found.\n", id); return; }
    /* shift left */
    for (int i = idx; i < p->count - 1; ++i) p->items[i] = p->items[i+1];
    p->count--;
    if (p->count == 0) {
        free(p->items);
        p->items = NULL;
    } else {
        Asset *tmp = realloc(p->items, sizeof(Asset) * p->count);
        if (tmp) p->items = tmp;
    }
    printf("Removed asset ID %d.\n", id);
}

void buy_sell(Portfolio *p, int is_buy) {
    int id = read_int(is_buy ? "Enter asset ID to buy: " : "Enter asset ID to sell: ");
    Asset *a = find_asset_by_id(p, id);
    if (!a) { printf("Asset ID %d not found.\n", id); return; }
    double qty = read_double(is_buy ? "Quantity to buy: " : "Quantity to sell: ");
    if (qty <= 0) { printf("Quantity must be positive.\n"); return; }
    if (!is_buy && qty > a->quantity) {
        printf("Cannot sell more than you own (owned: %.4f).\n", a->quantity);
        return;
    }
    if (is_buy) a->quantity += qty; else a->quantity -= qty;
    printf("%s %.4f units of %s. New quantity: %.4f\n", is_buy ? "Bought" : "Sold", qty, a->name, a->quantity);
}

void update_price(Portfolio *p) {
    int id = read_int("Enter asset ID to update price: ");
    Asset *a = find_asset_by_id(p, id);
    if (!a) { printf("Asset ID %d not found.\n", id); return; }
    double new_price = read_double("New price per unit: ");
    if (new_price < 0) { printf("Price cannot be negative.\n"); return; }
    a->price = new_price;
    printf("Updated price of %s to %.4f\n", a->name, a->price);
}

void view_portfolio(Portfolio *p) {
    if (p->count == 0) { printf("Portfolio is empty.\n"); return; }
    double total = 0.0;
    printf("ID  Name                          Type          Qty         Price       Value\n");
    printf("---------------------------------------------------------------------------\n");
    for (int i = 0; i < p->count; ++i) {
        Asset *a = &p->items[i];
        double value = a->quantity * a->price;
        total += value;
        printf("%-3d %-28s %-12s %10.4f %11.4f %11.4f\n",
               a->id, a->name, a->type, a->quantity, a->price, value);
    }
    printf("---------------------------------------------------------------------------\n");
    printf("Total portfolio value: %.4f\n", total);
}

/* --- Persistence (CSV) --- */
void save_portfolio(Portfolio *p) {
    FILE *f = fopen(FILE_NAME, "w");
    if (!f) { perror("Failed to open file for writing"); return; }
    /* CSV header */
    fprintf(f, "id,name,type,quantity,price\n");
    for (int i = 0; i < p->count; ++i) {
        Asset *a = &p->items[i];
        /* Escape commas in names/types minimally by replacing with space */
        char name_safe[NAME_LEN]; char type_safe[TYPE_LEN];
        strncpy(name_safe, a->name, NAME_LEN); name_safe[NAME_LEN-1] = '\0';
        strncpy(type_safe, a->type, TYPE_LEN); type_safe[TYPE_LEN-1] = '\0';
        for (char *s = name_safe; *s; ++s) if (*s == ',') *s = ' ';
        for (char *s = type_safe; *s; ++s) if (*s == ',') *s = ' ';
        fprintf(f, "%d,%s,%s,%.10g,%.10g\n", a->id, name_safe, type_safe, a->quantity, a->price);
    }
    fclose(f);
    printf("Portfolio saved to %s\n", FILE_NAME);
}

void load_portfolio(Portfolio *p) {
    FILE *f = fopen(FILE_NAME, "r");
    if (!f) {
        /* No file is OK */
        return;
    }
    char line[512];
    /* Read header line */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return; }
    /* Clear existing portfolio */
    free_portfolio(p);
    p->next_id = 1;
    while (fgets(line, sizeof(line), f)) {
        /* parse CSV: id,name,type,quantity,price */
        Asset a;
        char name[NAME_LEN], type[TYPE_LEN];
        int id;
        double qty, price;
        /* naive CSV parse assuming no commas in fields (we replaced them on save) */
        int fields = sscanf(line, "%d,%63[^,],%31[^,],%lf,%lf", &id, name, type, &qty, &price);
        if (fields == 5) {
            a.id = id;
            strncpy(a.name, name, NAME_LEN); a.name[NAME_LEN-1] = '\0';
            strncpy(a.type, type, TYPE_LEN); a.type[TYPE_LEN-1] = '\0';
            a.quantity = qty;
            a.price = price;
            Asset *tmp = realloc(p->items, sizeof(Asset) * (p->count + 1));
            if (!tmp) {
                printf("Memory allocation failed while loading file.\n");
                fclose(f);
                return;
            }
            p->items = tmp;
            p->items[p->count++] = a;
            if (a.id >= p->next_id) p->next_id = a.id + 1;
        }
    }
    fclose(f);
    if (p->count > 0) printf("Loaded %d assets from %s\n", p->count, FILE_NAME);
}

/* --- Demo data (optional) --- */
void seed_demo(Portfolio *p) {
    Asset demo[] = {
        {0, "ACME Corp", "Stock", 100.0, 12.34},
        {0, "Govt Bond 2030", "Bond", 50.0, 101.23},
        {0, "BigTech ETF", "ETF", 20.0, 250.0}
    };
    for (int i = 0; i < 3; ++i) {
        demo[i].id = p->next_id++;
        Asset *tmp = realloc(p->items, sizeof(Asset) * (p->count + 1));
        if (!tmp) return;
        p->items = tmp;
        p->items[p->count++] = demo[i];
    }
    printf("Demo assets added.\n");
}

/* --- Menu --- */
void print_menu(void) {
    printf("\n====== Portfolio Manager ======\n");
    printf("1. View portfolio\n");
    printf("2. Add asset\n");
    printf("3. Remove asset\n");
    printf("4. Buy asset (increase qty)\n");
    printf("5. Sell asset (decrease qty)\n");
    printf("6. Update asset price\n");
    printf("7. Save portfolio\n");
    printf("8. Load portfolio\n");
    printf("9. Seed demo data\n");
    printf("0. Exit\n");
    printf("===============================\n");
}

int main(void) {
    Portfolio p;
    init_portfolio(&p);
    load_portfolio(&p);

    int choice;
    while (1) {
        print_menu();
        choice = read_int("Enter choice: ");
        switch (choice) {
            case 1: view_portfolio(&p); break;
            case 2: add_asset(&p); break;
            case 3: remove_asset(&p); break;
            case 4: buy_sell(&p, 1); break;
            case 5: buy_sell(&p, 0); break;
            case 6: update_price(&p); break;
            case 7: save_portfolio(&p); break;
            case 8: free_portfolio(&p); init_portfolio(&p); load_portfolio(&p); break;
            case 9: seed_demo(&p); break;
            case 0:
                save_portfolio(&p);
                free_portfolio(&p);
                printf("Exiting. Goodbye.\n");
                return 0;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }
    return 0;
}