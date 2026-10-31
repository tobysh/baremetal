package main

import (
	"errors"
	"fmt"
	"sort"
	"strconv"
	"strings"
)

// Item is one stocked product.
type Item struct {
	SKU      string
	Qty      int
	Price    int // cents
	Reserved int
}

// Inventory holds the items of one stockroom.
type Inventory struct {
	items map[string]*Item
}

func NewInventory() *Inventory {
	return &Inventory{items: map[string]*Item{}}
}

func parseRecord(line string) (Item, error) {
	f := strings.Split(line, ",")
	if len(f) < 2 || len(f) > 4 {
		return Item{}, errors.New("want 2 or 3 fields")
	}
	sku := strings.TrimSpace(f[0])
	if sku == "" {
		return Item{}, errors.New("empty sku")
	}
	qty, err := strconv.Atoi(strings.TrimSpace(f[1]))
	if err != nil || qty < 0 {
		return Item{}, errors.New("bad quantity")
	}
	price := 0
	if len(f) >= 3 {
		price, err = strconv.Atoi(strings.TrimSpace(f[2]))
		if err != nil || price < 0 {
			return Item{}, errors.New("bad price")
		}
	}
	res := 0
	if len(f) == 4 {
		res, err = strconv.Atoi(strings.TrimSpace(f[3]))
		if err != nil || res < 0 || res > qty {
			return Item{}, errors.New("bad reserved")
		}
	}
	return Item{SKU: sku, Qty: qty, Price: price, Reserved: res}, nil
}

func isRecord(line string) bool {
	line = strings.TrimSpace(line)
	return line != "" && !strings.HasPrefix(line, "#")
}

// Load reads an inventory file (first-release and current shapes).
func Load(data string) (*Inventory, error) {
	inv := NewInventory()
	for i, line := range strings.Split(data, "\n") {
		line = strings.TrimSpace(strings.TrimSuffix(line, "\r"))
		if !isRecord(line) {
			continue
		}
		it, err := parseRecord(line)
		if err != nil {
			return nil, fmt.Errorf("line %d: %v", i+1, err)
		}
		if _, dup := inv.items[it.SKU]; dup {
			return nil, fmt.Errorf("line %d: duplicate sku", i+1)
		}
		cp := it
		inv.items[it.SKU] = &cp
	}
	return inv, nil
}

func formatItem(it Item) string {
	if it.Reserved > 0 {
		return fmt.Sprintf("%s,%d,%d,%d", it.SKU, it.Qty, it.Price, it.Reserved)
	}
	return fmt.Sprintf("%s,%d,%d", it.SKU, it.Qty, it.Price)
}

// Save writes the inventory, sorted by sku.
func (inv *Inventory) Save() string {
	skus := make([]string, 0, len(inv.items))
	for s := range inv.items {
		skus = append(skus, s)
	}
	sort.Strings(skus)
	var sb strings.Builder
	for _, s := range skus {
		sb.WriteString(formatItem(*inv.items[s]))
		sb.WriteByte('\n')
	}
	return sb.String()
}

// Adjust changes a quantity by delta (negative removes stock).
func (inv *Inventory) Adjust(sku string, delta int) error {
	if delta == 0 {
		return nil
	}
	it, ok := inv.items[sku]
	if !ok {
		if delta < 0 {
			return errors.New("unknown sku")
		}
		inv.items[sku] = &Item{SKU: sku, Qty: delta}
		return nil
	}
	if it.Qty+delta < it.Reserved {
		return errors.New("insufficient")
	}
	it.Qty += delta
	return nil
}

// SetPrice sets the unit price in cents.
func (inv *Inventory) SetPrice(sku string, cents int) error {
	it, ok := inv.items[sku]
	if !ok {
		return errors.New("unknown sku")
	}
	if cents < 0 {
		return errors.New("bad price")
	}
	it.Price = cents
	return nil
}

// Qty is the physical quantity (0 when unknown).
func (inv *Inventory) Qty(sku string) int {
	if it, ok := inv.items[sku]; ok {
		return it.Qty
	}
	return 0
}

// Value is the total value of physical stock, in cents.
func (inv *Inventory) Value() int {
	total := 0
	for _, it := range inv.items {
		total += it.Qty * it.Price
	}
	return total
}

// Low lists skus whose physical quantity is below threshold, lowest first.
func (inv *Inventory) Low(threshold int) []string {
	var low []*Item
	for _, it := range inv.items {
		if it.Qty < threshold {
			low = append(low, it)
		}
	}
	sort.Slice(low, func(i, j int) bool {
		if low[i].Qty != low[j].Qty {
			return low[i].Qty < low[j].Qty
		}
		return low[i].SKU < low[j].SKU
	})
	out := make([]string, len(low))
	for i, it := range low {
		out[i] = it.SKU
	}
	return out
}

// Reserve sets stock aside.
func (inv *Inventory) Reserve(sku string, n int) error {
	it, ok := inv.items[sku]
	if !ok || n <= 0 || it.Qty-it.Reserved < n {
		return errors.New("cannot reserve")
	}
	it.Reserved += n
	return nil
}

// Release returns reserved stock to the available pool.
func (inv *Inventory) Release(sku string, n int) error {
	it, ok := inv.items[sku]
	if !ok || n <= 0 || it.Reserved < n {
		return errors.New("cannot release")
	}
	it.Reserved -= n
	return nil
}

// Fulfil ships reserved stock.
func (inv *Inventory) Fulfil(sku string, n int) error {
	it, ok := inv.items[sku]
	if !ok || n <= 0 || it.Reserved < n {
		return errors.New("cannot fulfil")
	}
	it.Reserved -= n
	it.Qty -= n
	return nil
}

// Available is the quantity not reserved.
func (inv *Inventory) Available(sku string) int {
	if it, ok := inv.items[sku]; ok {
		return it.Qty - it.Reserved
	}
	return 0
}

// Upgrade rewrites a stored file into canonical form.
func Upgrade(data string) (string, error) {
	lines := strings.Split(data, "\n")
	seen := map[string]bool{}
	for i, line := range lines {
		line = strings.TrimSuffix(line, "\r")
		if !isRecord(line) {
			lines[i] = line
			continue
		}
		it, err := parseRecord(strings.TrimSpace(line))
		if err != nil {
			return "", fmt.Errorf("line %d: %v", i+1, err)
		}
		if seen[it.SKU] {
			return "", fmt.Errorf("line %d: duplicate sku", i+1)
		}
		seen[it.SKU] = true
		lines[i] = formatItem(it)
	}
	return strings.Join(lines, "\n"), nil
}

func main() {
	inv, err := Load("# stock\nbolt,40\nnut,3,15\n")
	fmt.Println(err, inv.Qty("bolt"), inv.Low(10), inv.Value())
	fmt.Print(inv.Save())
}
