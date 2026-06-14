#include <cstdint>
#include <cstring>
#include <cstdio>
#include <charconv>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

constexpr uint32_t GOLDEN = 0x9E3779B9U;
constexpr uint64_t GOLDEN64 = 0x9E3779B97F4A7C15ULL;

typedef uint32_t v128_t __attribute__((vector_size(16)));
typedef uint32_t v128_u __attribute__((vector_size(16), aligned(1), may_alias));
typedef uint64_t v128_64 __attribute__((vector_size(16)));

constexpr int LANES = 8;
constexpr size_t WIDTH = sizeof(v128_t);
constexpr size_t BLOCK_SIZE = LANES * WIDTH;

struct HashState {
	v128_t accumulator[LANES];
	
	HashState() {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = v128_t{GOLDEN * (4 * i + 1), GOLDEN * (4 * i + 2), GOLDEN * (4 * i + 3), GOLDEN * (4 * i + 4)};
	}
	
	void absorb(const uint8_t* queue) {
		constexpr v128_t golden_vec = {GOLDEN, GOLDEN, GOLDEN, GOLDEN};
		
		for (int i = 0; i < LANES; i++) {
			const v128_u block = *(const v128_u*)(queue + i * WIDTH);
			v128_64 acc = (v128_64)((accumulator[i] ^ block) * golden_vec);
			accumulator[i] = (v128_t)(acc ^ acc >> 16 ^ acc << 32) * golden_vec;
		}
	}
	
	uint64_t digest(const uint8_t* tail, const size_t tail_n) {
		uint8_t block[BLOCK_SIZE] = {};
		memcpy(block, tail, tail_n);
		absorb(block);
		uint64_t hash = tail_n * GOLDEN64;
		
		for (int i = 0; i < LANES; i++) {
			const v128_64 acc = (v128_64)accumulator[i];
			hash ^= acc[0];
			hash = (hash ^ hash >> 32) * GOLDEN64;
			hash ^= acc[1];
			hash = (hash ^ hash >> 32) * GOLDEN64;
		}
		
		hash = (hash ^ hash >> 32) * GOLDEN64;
		return hash ^ hash >> 32;
	}
};

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return 1;
	}
	
	const int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror(argv[1]);
		return 1;
	}
	
	struct stat info;
	if (fstat(fd, &info) < 0) {
		perror("fstat");
		return 1;
	}
	
	const size_t n = info.st_size;
	const uint8_t* data = (const uint8_t*)"";
	
	if (n) {
		data = (const uint8_t*)mmap(nullptr, n, PROT_READ, MAP_PRIVATE, fd, 0);
		if (data == MAP_FAILED) {
			perror("mmap");
			return 1;
		}
		madvise((void*)data, n, MADV_SEQUENTIAL);
	}
	
	HashState state;
	size_t offset;
	
	for (offset = 0; n - offset >= BLOCK_SIZE; offset += BLOCK_SIZE)
		state.absorb(data + offset);
	
	const uint64_t hash = state.digest(data + offset, n - offset);
	
	char buffer[14];
	char* ptr = std::to_chars(buffer, buffer + sizeof(buffer), hash, 36).ptr;
	*ptr = '\0';
	puts(buffer);
}
