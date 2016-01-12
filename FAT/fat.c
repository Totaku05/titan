#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define DIR "/home/totaku/sample_git1/FAT/fs" 

#define COUNT 2048
#define SIZE 2048
#define LEN 150
#define NUM 128

char *buf;
FILE *f;
size_t sizeint = 4;
int i;
int res;

int create_system();

typedef struct description {
	char free;
	char dir;
	int first;
	int size;
	char name[LEN];
} d_t;

typedef struct system {
	int next[COUNT];
	d_t d[NUM];
} s_t;

int getdata(d_t *fd, char **buf);
s_t s;
size_t sd = sizeof(d_t);
static int fat_getattr(const char *path, struct stat *stbuf);

static int fat_create (const char *path, mode_t mode, struct fuse_file_info *fi);

static int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

static int fat_open(const char *path, struct fuse_file_info *fi);

static int fat_opendir(const char *path, struct fuse_file_info *fi);

static int fat_mkdir(const char *path, mode_t mode);

static int fat_unlink(const char *path);

static int fat_write(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

static int fat_init(struct fuse_conn_info *fi);

static int fat_rmdir(const char *path);

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi);

static int fat_truncate(const char *path, off_t offset);

static int fat_rename(const char *path, const char *new_path);

static struct fuse_operations operations = {
	.getattr	= fat_getattr,//
	.readdir	= fat_readdir,//
	.open		= fat_open,//
	.read		= fat_read,//
	.mkdir		= fat_mkdir,//
	.rmdir		= fat_rmdir,//
	.create		= fat_create,//
	.unlink		= fat_unlink,
	.write		= fat_write,//
	.opendir	= fat_opendir,//
	.init		= fat_init,//
	.rename		= fat_rename,
	.truncate	= fat_truncate,//
};

int main(int argc, char *argv[])
{
	create_system();
	fuse_main(argc, argv, &operations, NULL);
	return 0;
}

int create_system() {
	f = fopen(DIR, "w+");
	buf = (char*)malloc(sd);
	memset(buf, '\0', sd);
	for (i = 0; i < COUNT; i++) {
		fwrite(buf, sd, 1, f);
	}

	buf = (char*)malloc(sizeint);
	memset(buf, '\0', sizeint);
	for (i = 0; i < COUNT; i++) {
		fwrite(buf, sizeint, 1, f);
	}

	buf = (char*)malloc(SIZE);
	memset(buf, '\0', SIZE);
	for (i = 0; i < COUNT; i++) {
		fwrite(buf, SIZE, 1, f);
	}

	fclose(f);
	init();
	add_file("/", buf, 0, 1);
	return 0;
}

void init() {
	for (i = 0; i < NUM; i++) {
		memset(s.d[i].name, '\0', LEN);
		s.d[i].first = -100;
		s.d[i].size = 0;
		s.d[i].free = 1;
		s.d[i].dir = 0;
	}
	for (i = 0; i < COUNT; i++) {
		s.next[i] = -100;
	}
}

void add_file(char *name, char *data, int size, char dir) {
	d_t *fd = NULL;
	int first = 0; 
	int d_id = -1;
	for (i = 0; i < NUM; i++) {
		if (s.d[i].free)
			d_id = i;
	}
	if (d_id == -1)
		return;
	fd = &s[d_id];
	fd->free = 0;
	fd->dir = dir;
	fd->size = size;
	strcpy(fd->name, name);
	first = -1;
	for (i = 0; i < COUNT; i++) {
		if (s.next[i] == -100)
			first = i;
	}
	if (first == -1)
		return;
	fd->first = first;
	s.next[first] = -1;
	write_d(fd, d_id);	
	f = fopen(DIR, "r+");
	i = fd->first;
	fseek(f, sd * NUM + i * sizeint, SEEK_SET);
	fwrite(&s.next[i], sizeint, 1, f);
	while (i != -100) {
		i = s.next[i];
		fseek(f, sd * NUM + i * sizeint, SEEK_SET);
		fwrite(&s.next[i], sizeint, 1, f);
	}
	fclose(f);
}

void write_d(d_t *fd, int id) {
	f = fopen(DIR, "r+");
	fseek(f, id * sd, SEEK_SET);
	fwrite(fd, 1, sd, f);
	fclose(f);
}

int getd(char *path, d_t **fd) {
	int size = 0;
	d_t *meta = NULL;
	int metaid = -1;
	int id = -1;
	char name[LEN], *data, *first, *str = path;
	memset(name, '\0', LEN);
	if (strcmp(path, "/") == 0) {
		*fd = &s.d[0];
		return 0;
	}
	if (*str++ == '/')
		meta = &s.d[0];
	else 
		return -100;
	while (str - path != strlen(path)) {
		if (meta->size == 0)
			return NULL;
		size = getdata(meta, &data);
		meta = NULL;
		first = str;
		while ((*str++ != '/') && (str - path < strlen(path)));
		str - path < strlen(path) ? strncpy(name, first, str - first - 1) : strncpy(name, first, str - first);
		for (i = 0; i < size / sizeint; i++)
			if (strcmp(s.d[((int*)data)[i]].name, name) == 0)
				id = ((int*)data)[i];
		if (id == -1)
			return -100;
		meta = &s.d[id];
		memset(name, '\0', LEN);
		free(data);
	}
	*fd = meta;
	return id;
}

int getdata(d_t *fd, char **buf) {
	f = fopen(DIR, "r");
	char *data = NULL;
	int current = 0;
	int b = 0;
	int count = 0;
	if (fd == NULL)
		return -1;
	data = (char*)malloc(fd->size);
	current = fd->first;
	while (current > 0) {
		count = (fd->size - b * SIZE) > SIZE ? SIZE : (fd->size - b * SIZE);
		fseek(f, sd * NUM + COUNT * sizeint + current * SIZE, SEEK_SET); 
		fread(data + b * SIZE, 1, count, f);
		b++;
		current = s.next[current];
	}
	fclose(f);
	*buf = data;
	return fd->size;
}

static int fat_getattr(const char *path, struct stat *stbuf)
{
	res = 0;
	char *data;
	d_t *meta;
	if (getd(path, &meta) == -100)
		return -100;
	memset(stbuf, 0, sizeof(struct stat));
	if (meta->dir == 1) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} 
	else {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = meta->size;
	};
	stbuf->st_mode = stbuf->st_mode | 0777;
	return res;
}

int write_data(d_t *fd, void *data, int size) {
	int j = 0, b;
	if (size == 0)
		return 0;
	f = fopen(DIR, "r+");
	i = fd->first;
	b = SIZE;
	while (i != -1) {
		b = ((j + 1) * SIZE) > size ? size % SIZE : b;
		fseek(f, sd * NUM + sizeint * COUNT + i * SIZE, SEEK_SET);
		fwrite(data + j * SIZE, 1, 1 * b, f);
		i = s.next[i];
		j++;
	}
	fd->size = size;
	i = fd->first;
	fseek(f, sd * NUM + i * sizeint, SEEK_SET);
	fwrite(&s.next[i], sizeint, 1, f);
	while (i != -100) {
		i = s.next[i];
		fseek(f, sd * NUM + i * sizeint, SEEK_SET);
		fwrite(&s.next[i], sizeint, 1, f);
	}
	fclose(f);
	return 0;
}

int make(const char *path, int type) {
	int i = 0;
	int fd = 0, size = 0, id = -1;
	char *dir;
	char *name;
	int *data, *mdata;
	d_t *meta;

	char *p = NULL, *str = path;
	while (p = *str == '/' ? str : p, *str++ != '\0');
	if (p - path != 0) {
		dir = (char*)malloc(p - path);
		strncpy(dir, path, p - path);
		dir[p - path] = '\0';
	}
	else {
		dir = (char*)malloc(2);
		strcpy(dir, "/\0");
	}

	str = path;
	name = (char*)malloc(str - p);
	strncpy(name, p + 1, str - p);

	id = getd(dir, &meta);
	size = getdata(meta, &data);
	mdata = (char*)malloc(size + sizeint);
	memcpy(mdata, data, size);
	fd = add_file(name, NULL, 0, type);
	mdata[size/sizeint] = fd;
	write_data(meta, mdata, size + sizeint);
	meta->size = size + sizeint;
	write_d(meta, id);
	return 0;

}

static int fat_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
	res = make(path, 0);
	if (res != 0)
		return -100;
	return 0;
}

static int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	char *data;
	int size;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	d_t *meta;

	if (getd(path, &meta) == -100)
		return -100;

	size = getdata(meta, &data);

	if (meta->size == 0)
		return 0;

	for (i = 0; i < size / sizeint; i++)
		filler(buf, s.d[((int*)data)[i]].name, NULL, 0);
	return 0;
}

static int fat_open(const char *path, struct fuse_file_info *fi)
{
	d_t *meta;
	if (getd(path, &meta) == -100)
		return -100;
	return 0;
}

static int fat_opendir(const char *path, struct fuse_file_info *fi) {
	d_t *meta;
	if (getd(path, &meta) == -100)
		return -100;
	return 0;
}

static int fat_mkdir(const char *path, mode_t mode) {
	if (make(path, 1) != 0)
		return -100;
	return 0;
}

int remove(const char *path) {
	int j = 0;
	int fd = 0, size = 0, dir_id = -1, f_id = -1;
	char *p = NULL, *str = path;
	char *dir;
	int *data, *mdata;
	d_t *dir_d, *f_d;
	while (p = *str == '/' ? str : p, *str++ != '\0');
	if (p - path != 0) {
		dir = (char*)malloc(p - path);
		strncpy(dir, path, p - path);
		dir[p - path] = '\0';
	}
	else {
		dir = (char*)malloc(2);
		strcpy(dir, "/\0");
	}
	dir_id = getd(dir, &dir_d);
	f_id = getd(path, &f_d);
	size = getdata(dir_d, &data);

	mdata = (char*)malloc(size - sizeint);
	for (i = 0; i < size/sizeint; i++)
		if (data[i] != f_id)
			mdata[j++] = data[i];
	write_data(dir_d, mdata, size - sizeint);
	dir_d->size = size - sizeint;
	write_d(dir_d, dir_id);
	return 0;
}

static int fat_unlink(const char *path) {
	if (remove(path) != 0)
		return -100;
	return 0;
}

static int fat_write(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	d_t *fd;
	int d_id = getd(path, &fd);
	if (d_id == -100)
		return -100;
	res = write_data(fd, buf, size);
	write_d(fd, d_id);
	if (res != 0)
		return -100;
	return 0;
}

static int fat_init(struct fuse_conn_info *fi) {
	init();
	f = fopen(DIR,"r");
	fread(s.d, sd, NUM, f);
	fread(s.next, sizeint, COUNT, f);
	for (i = 0; i < COUNT; i++)
		if (s.next[i] == 0)
			s.next[i] = -100;
}

static int fat_rmdir(const char *path) {
	if (remove(path) != 0)
		return -100;
	return 0;
}

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	char *data;
	d_t *meta;
	getd(path, &meta);
	if (meta == NULL)
		return -100;

	len = getdata(meta, &data);

	if (len < 0)
		return -100;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, data + offset, size);

	}
	else
		size = 0;
	return size;
}

int truncate(d_t *fd, int offset) {
	if (offset == 0)
		return 0;
	int current, count, previous = 0, w = 0, last = 0;
	i = 0;
	count = offset / SIZE;
	current = fd->first;
	if (count != 0) {
		while (current != -1) {
			previous = current;
			current = s.next[i];
		}
		while (w != 0) {
			current = -100;
			for (i = 0; i < COUNT; i++) {
				if (s.next[i] == -100)
					current = i;
			}
			if (current == -100) 
				return -100;
			s.next[previous] = current;
			previous = current;
			w--;
		}
	}
	last = offset % SIZE;
	if (last != 0 && count != 0) {
		for (i = 0; i < COUNT; i++) {
			if (s.next[i] == -100)
				current = i;
		}
		s.next[previous] = current;
	}
	s.next[current] = -100;
	fd->size = fd->size + offset;
	return 0;
}

static int fat_truncate(const char *path, off_t offset) {
	if (offset == 0)
		return 0;	
	d_t *fd = NULL;
	int d_id = 0;
	d_id = getd(path, &fd);
	if (d_id == -100);
		return -100;
	truncate(fd, offset);
	write(fd, d_id);
	return 0;
}

static int fat_rename(const char *path, const char *new_path) {
	char *dir = NULL, *name = NULL;
	d_t *dir_d = NULL;
	int dir_id = 0;
	int *dir_data, *dir_ndata;
	d_t *fd = NULL;
	int d_id = 0;

	char *p = NULL, *str = new_path;
	while (p = *str == '/' ? str : p, *str++ != '\0');
	if (p - new_path != 0) {
		dir = (char*)malloc(p - new_path);
		strncpy(dir, new_path, p - new_path);
		dir[p - new_path] = '\0';
	}
	else {
		dir = (char*)malloc(2);
		strcpy(dir, "/\0");
	}

	str = new_path;
	name = (char*)malloc(str - p);
	strncpy(name, p + 1, str - p);

	d_id = getd(path, &fd);
	strcpy(fd->name, name);
	write_d(fd, d_id);
	
	remove(path);
	

	dir_id = getd(dir, &dir_d);
	getdata(dir_d, &dir_data);

	dir_ndata = (int*)malloc(dir_d->size + sizeint);
	dir_ndata[dir_d->size/sizeint] = d_id;

	write_data(dir_d, dir_ndata, dir_d->size + sizeint);
	dir_d->size += sizeint;
	write_fd(dir_d, dir_id);
	return 0;
}
