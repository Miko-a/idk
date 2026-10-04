# Penjelasan Program Cache DadJokes

Program di `main.go` mengambil lelucon dari API [icanhazdadjoke.com](https://icanhazdadjoke.com/) secara bersamaan. Setelah lelucon diterima, program menyimpan hasil yang sudah diproses ke dalam cache.

Tujuannya sederhana: bila respons yang sama datang lagi, program tidak perlu membaca JSON dari awal lagi. Program cukup mengambil teks lelucon yang sudah tersimpan di cache.

## Gambaran alur program

1. Program membuat 50 goroutine. Goroutine adalah pekerjaan kecil yang dapat berjalan bersamaan.
2. Setiap goroutine meminta satu DadJoke ke API.
3. API mengirim respons dalam bentuk JSON, lalu program membacanya menjadi `bodyBytes`.
4. Isi `bodyBytes` dijadikan kunci cache.
5. Jika kunci sudah ada di cache (**cache hit**), program mengambil lelucon yang sudah jadi dari `Data.Payload`.
6. Jika kunci belum ada (**cache miss**), program menjalankan `json.Unmarshal`, mengambil `responseObject.Joke`, lalu menyimpannya ke cache.
7. Semua goroutine selesai, lalu program mencetak waktu proses serta jumlah hit dan miss.

Secara singkat, alurnya seperti ini:

```text
50 goroutine
    |
    v
Request ke DadJokes
    |
    v
bodyBytes sebagai key cache
    |
    +-- key ada  -> HIT  -> ambil Data.Payload
    |
    +-- key tidak ada -> MISS -> json.Unmarshal -> simpan Data.Payload
```


### `Response`

```go
type Response struct {
    ID     string `json:"id"`
    Joke   string `json:"joke"`
    Status int    `json:"status"`
}
```

Struktur ini adalah bentuk data JSON yang dikirim API. Bagian yang paling kita perlukan adalah `Joke`, karena isinya teks lelucon.

Contoh bentuk respons API:

```json
{
  "id": "abc123",
  "joke": "Why did the chicken cross the road?",
  "status": 200
}
```

Setelah JSON dibaca dengan `json.Unmarshal`, teks lelucon dapat diakses melalui `responseObject.Joke`.

### `Data`

```go
type Data struct {
    ID      string
    Payload string
}
```

`Data` adalah isi yang disimpan di cache.

- `ID` berisi key cache, yaitu hasil konversi `bodyBytes` menjadi string.
- `Payload` berisi lelucon yang sudah diproses, yaitu `responseObject.Joke`.

Jadi saat cache ditemukan, program cukup memakai `Payload`. Tidak perlu memanggil `json.Unmarshal` lagi.

### `Cache`

```go
type Cache struct {
    mu   sync.Mutex
    m    map[string]*Data
    hit  int
    miss int
}
```

Cache mempunyai empat bagian:

- `mu`: kunci pengaman. Karena 50 goroutine dapat membuka cache hampir bersamaan, bagian ini mencegah dua goroutine mengubah map atau counter pada waktu yang sama.
- `m`: tempat penyimpanan cache. Bentuknya `map`, seperti daftar pasangan `key -> data`.
- `hit`: penghitung cache yang berhasil ditemukan.
- `miss`: penghitung cache yang belum ditemukan dan harus diproses.

Tanpa `sync.Mutex`, akses map dari banyak goroutine dapat saling bertabrakan dan program bisa mengalami error seperti `concurrent map writes`.

### Fungsi `Cache.Get`

```go
func (c *Cache) Get(bodyBytes []byte) (Data, bool)
```

Fungsi ini menerima `bodyBytes` dari API dan mengembalikan dua nilai:

- `Data`: data lelucon dari cache atau hasil proses baru.
- `bool`: `true` bila berhasil diproses, `false` bila JSON tidak valid.

Langkah di dalamnya:

```go
key := string(bodyBytes)
```

Seluruh isi respons API diubah menjadi string dan digunakan sebagai key. Cara ini sesuai kebutuhan program. Pada aplikasi besar, key yang panjang biasanya dapat diubah menjadi hash supaya lebih ringkas.

Lalu program mengunci cache:

```go
c.mu.Lock()
defer c.mu.Unlock()
```

Kunci dibuka otomatis saat fungsi selesai, termasuk bila fungsi keluar lebih awal dengan `return`.

#### Kondisi cache hit

```go
if data, exists := c.m[key]; exists {
    c.hit++
    return *data, true
}
```

Artinya key yang sama sudah pernah diproses.

Program menaikkan `hit`, lalu langsung mengembalikan `*data`. Tanda `*` berarti mengambil isi dari pointer `Data`. Bagian ini tidak menjalankan `json.Unmarshal` lagi.

#### Kondisi cache miss

```go
var responseObject Response
if err := json.Unmarshal(bodyBytes, &responseObject); err != nil {
    return Data{}, false
}
```

Jika key belum tersedia, barulah program menerjemahkan JSON ke struktur `Response`. Setelah itu, program membuat data cache:

```go
data := &Data{
    ID:      key,
    Payload: responseObject.Joke,
}
c.m[key] = data
c.miss++
```

`Payload` mengambil isi `responseObject.Joke`. Data ini dimasukkan ke map menggunakan `key`, kemudian penghitung `miss` dinaikkan.

### Fungsi `main`

```go
cache := Cache{m: make(map[string]*Data)}
client := &http.Client{Timeout: 10 * time.Second}
```

Program membuat cache kosong dan satu HTTP client. Satu client dipakai bersama agar koneksi HTTP bisa dikelola lebih efisien.

```go
var wg sync.WaitGroup
wg.Add(numberOfRequests)
```

`WaitGroup` dipakai seperti penghitung tugas. Karena ada 50 pekerjaan yang harus selesai, program menambahkan angka 50 terlebih dahulu.

```go
for i := 0; i < numberOfRequests; i++ {
    go getBadJoke(&wg, client, &cache)
}
wg.Wait()
```

Perulangan ini membuat 50 goroutine. Kata `go` membuat pemanggilan fungsi berjalan sendiri-sendiri secara bersamaan.

`wg.Wait()` menahan fungsi `main` agar tidak selesai terlalu cepat. Program hanya lanjut mencetak statistik setelah seluruh goroutine memanggil `wg.Done()`.

### Fungsi `getBadJoke`

Fungsi ini dikerjakan oleh setiap goroutine.

```go
defer wg.Done()
```

Baris ini sangat penting. Ketika fungsi selesai, baik karena berhasil maupun karena error, jumlah tugas pada `WaitGroup` dikurangi satu.

Selanjutnya fungsi membuat request HTTP:

```go
request, err := http.NewRequest(http.MethodGet, "https://icanhazdadjoke.com/", nil)
request.Header.Set("Accept", "application/json")
```

Header `Accept: application/json` meminta agar DadJokes mengirim respons JSON, bukan halaman HTML.

Respons API dibaca menjadi byte:

```go
bodyBytes, err := io.ReadAll(response.Body)
```

Kemudian byte tersebut diberikan ke cache:

```go
data, ok := cache.Get(bodyBytes)
```

Jika berhasil, program mencetak hasil yang ada di `data.Payload`:

```go
fmt.Println(data.Payload)
```

Nilai ini bisa berasal dari dua tempat:

- hasil `json.Unmarshal` baru saat cache miss; atau
- data yang sudah tersimpan saat cache hit.

## Arti hasil akhir

Di akhir program akan muncul keluaran seperti berikut:

```text
Processes took 1.14s
Cache hit: 1
Cache miss: 49
```

Artinya 50 goroutine selesai dalam sekitar 1,14 detik. Ada satu respons API yang sama sehingga bisa diambil dari cache, sedangkan 49 respons lain berbeda sehingga perlu diterjemahkan dari JSON dan disimpan sebagai data baru.

Jumlah hit tidak selalu sama pada setiap eksekusi. DadJokes biasanya mengirim lelucon acak, sehingga bisa saja semua respons berbeda dan hasilnya menjadi `Cache hit: 0` serta `Cache miss: 50`.

## Kode Program
```go
package main

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"sync"
	"time"
)

const numberOfRequests = 50

type Response struct {
	ID     string `json:"id"`
	Joke   string `json:"joke"`
	Status int    `json:"status"`
}

type Data struct {
	ID      string
	Payload string
}

type Cache struct {
	mu   sync.Mutex
	m    map[string]*Data
	hit  int
	miss int
}

func (c *Cache) Get(bodyBytes []byte) (Data, bool) {
	key := string(bodyBytes)

	c.mu.Lock()
	defer c.mu.Unlock()

	if data, exists := c.m[key]; exists {
		c.hit++
		return *data, true
	}

	var responseObject Response
	if err := json.Unmarshal(bodyBytes, &responseObject); err != nil {
		return Data{}, false
	}

	data := &Data{
		ID:      key,
		Payload: responseObject.Joke,
	}
	c.m[key] = data
	c.miss++
	return *data, true
}

func (c *Cache) Stats() (hit, miss int) {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.hit, c.miss
}

func main() {
	start := time.Now()
	cache := Cache{m: make(map[string]*Data)}
	client := &http.Client{Timeout: 10 * time.Second}

	var wg sync.WaitGroup
	wg.Add(numberOfRequests)
	for i := 0; i < numberOfRequests; i++ {
		go getBadJoke(&wg, client, &cache)
	}
	wg.Wait()

	hit, miss := cache.Stats()
	fmt.Printf("Processes took %s\n", time.Since(start))
	fmt.Printf("Cache hit: %d\n", hit)
	fmt.Printf("Cache miss: %d\n", miss)
}

func getBadJoke(wg *sync.WaitGroup, client *http.Client, cache *Cache) {
	defer wg.Done()

	request, err := http.NewRequest(http.MethodGet, "https://icanhazdadjoke.com/", nil)
	if err != nil {
		fmt.Println(err)
		return
	}
	request.Header.Set("Accept", "application/json")

	response, err := client.Do(request)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer response.Body.Close()

	bodyBytes, err := io.ReadAll(response.Body)
	if err != nil {
		fmt.Println(err)
		return
	}

	data, ok := cache.Get(bodyBytes)
	if !ok {
		fmt.Println("failed to process DadJokes response")
		return
	}

	fmt.Println(data.Payload)
}
```


## Screenshot
<img width="992" height="494" alt="pasted file" src="https://github.com/user-attachments/assets/7ca035bf-57b8-457d-a290-5c764aed6642" />

<img width="992" height="494" alt="pasted file(1)" src="https://github.com/user-attachments/assets/916dfab2-de94-4694-a842-89117fa440d2" />


