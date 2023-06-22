#include "linux/jiffies.h"
#include "linux/cpuset.h"
#include <linux/sched/numa_balancing.h>
#include "linux/mm.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/delay.h>

MODULE_DESCRIPTION("Numa dump module");
MODULE_AUTHOR("Jérôme Coquisart, LIP6");
MODULE_LICENSE("GPL");

#define LINE_SIZE 256
#define BUFFER_NODE 10

static struct dentry *numa_dump_dir;
static struct dentry *numa_dump_start_file;
static struct dentry *numa_dump_show_log;
static struct dentry *numa_dump_stop_file;
static struct dentry *numa_dump_manual_log;

static int pid = -1;
module_param(pid, int, 0660);
static unsigned long addr;
module_param(addr, ulong, 0660);
static size_t size;
module_param(size, ulong, 0660);
static int pagecache_node;
module_param(pagecache_node, int, 0660);

static bool paused = false;
static int buffer_nodes[BUFFER_NODE];

static struct dump {
	char dmp[LINE_SIZE];
	struct dump *next;
} *dumps, *curr_dump;

static int number_dumps;

static void free_dumps(struct dump *dump)
{
	struct dump *next;

	for (int i = 0; i < BUFFER_NODE; i++) {
		buffer_nodes[i] = 0;
	}

	if (!dump)
		return;

	while (dump->next) {
		next = dump->next;
		pr_debug("freeing %p", dump);
		kfree(dump);
		dump = next;
		number_dumps--;
	}
	curr_dump = NULL;
	dumps = NULL;
}

static void dump_numa_info(struct dump *dump, bool dump_pages,
			   int nr_migrations, int src_nid)
{
	struct page **pages;
	struct task_struct *ts = NULL;
	unsigned long time, total_numa_faults, numa_pages_migrated;
	int numa_preferred_nid, read_node, page_cache_node;
	int number_of_nodes = num_possible_nodes();
	int pages_found, nr_pages = size / PAGE_SIZE;
	int nid, flags;
	char *log_string;
	int n;

	nr_pages = max(1, nr_pages);

	if (pid < 1) {
		pr_err("pid should be positive");
		goto out;
	}

	if ((void *)addr == NULL) {
		pr_err("addr is null");
		goto out;
	}

	ts = current;

	time = jiffies64_to_msecs(jiffies);
	numa_preferred_nid = ts->numa_preferred_nid;
	read_node = numa_node_id();
	page_cache_node = pagecache_node;
	total_numa_faults = ts->total_numa_faults;
	numa_pages_migrated = ts->numa_pages_migrated;

	if (dump_pages) {
		pages = kmalloc(sizeof(struct page *) * nr_pages, GFP_KERNEL);
		if (pages == NULL) {
			pr_err("failed allocation of pages");
			goto out;
		}
		flags = FOLL_NOFAULT | FOLL_FORCE | FOLL_GET;
		pages_found = get_user_pages_fast(addr & PAGE_MASK, nr_pages,
						  flags, pages);

		for (int i = 0; i < pages_found; i++) {
			nid = page_to_nid(pages[i]);
			put_page(pages[i]);
			buffer_nodes[nid]++;
		}
		kfree(pages);
	}
	if (nr_migrations) {
		buffer_nodes[read_node] += nr_migrations;
		buffer_nodes[src_nid] -= nr_migrations;
	}

	log_string = dump->dmp;
	n = snprintf(
		log_string, sizeof(char) * LINE_SIZE,
		"{ \"time\" : %ld, \"numa_preferred_nid\" : %d, \"read_node\" : %d, \"pagecache_node\" : %d, \"total_numa_faults\" : %ld, \"numa_pages_migrated\" : %ld, \"buffer_nodes\" : [",
		time, numa_preferred_nid, read_node, page_cache_node,
		total_numa_faults, numa_pages_migrated);

	for (int i = 0; i < number_of_nodes; i++) {
		n += snprintf(log_string + n, sizeof(char) * LINE_SIZE - n,
			      "%d,", buffer_nodes[i]);
	}
	n--; // remove the trailing ,
	n += snprintf(log_string + n, sizeof(char) * LINE_SIZE - n, "]}");
	memset(log_string + n, ' ', LINE_SIZE - n);
	log_string[LINE_SIZE - 1] = '\n';

	pr_debug("logs: %s", log_string);

out:
	return;
}

void log_event_numa(bool dump_pages, int nr_migrations, int src_nid)
{
	struct dump *dmp;

	if (!(current->pid == pid) || paused)
		return;

	dmp = kmalloc(sizeof(struct dump), GFP_KERNEL);

	if (dmp == NULL) {
		pr_err("failed allocation of dmp");
		return;
	}

	if (!dumps) {
		dumps = dmp;
		dumps->next = NULL;
		curr_dump = dumps;
		number_dumps++;
	} else if (!curr_dump->next) {
		number_dumps++;
		curr_dump->next = dmp;
		curr_dump->next->next = NULL;
		curr_dump = curr_dump->next;
	}
	dump_numa_info(curr_dump, dump_pages, nr_migrations, src_nid);
}
EXPORT_SYMBOL(log_event_numa);

static ssize_t read_dumps(struct file *file, char __user *buf, size_t count,
			  loff_t *pos)
{
	struct dump *read_dmp = dumps;
	int start_dump = *pos / LINE_SIZE;
	unsigned long start_dump_char = *pos % LINE_SIZE;
	ssize_t bytes_copied = 0;
	size_t bytes_to_copy = count;
	int n;

	if (!dumps)
		return -1;

	if (start_dump > number_dumps)
		return 0;

	/* find the right dump for fetching data */
	for (int i = 0; i < start_dump; i++) {
		if (!read_dmp->next)
			return 0;
		read_dmp = read_dmp->next;
	}

	while (bytes_to_copy > 0 && bytes_copied < count) {
		size_t z = min(bytes_to_copy, LINE_SIZE - start_dump_char);
		n = copy_to_user(buf + bytes_copied,
				 read_dmp->dmp + start_dump_char, z);
		if (n < 0)
			return -EFAULT;
		n = z - n;

		bytes_to_copy -= n;
		bytes_copied += n;

		if (!read_dmp->next || n == 0) {
			break;
		}
		read_dmp = read_dmp->next;
		start_dump_char = 0;
	}

	*pos += bytes_copied;
	return bytes_copied;
}

static ssize_t write_stop(struct file *file, const char __user *buf,
			  size_t count, loff_t *pos)
{
	pr_debug("dump paused\n");
	paused = true;
	pos += count;
	return count;
}

static ssize_t write_start(struct file *file, const char __user *buf,
			   size_t count, loff_t *pos)
{
	pr_debug("starting logging with pid %d\n", current->pid);
	free_dumps(dumps);
	paused = false;
	log_event_numa(true, 0, 0);
	pos += count;
	return count;
}

static ssize_t manual_log(struct file *file, const char __user *buf,
			  size_t count, loff_t *pos)
{
	log_event_numa(false, 0, 0);
	return 0;
}

static const struct file_operations start_file_ops = {
	.write = write_start,
};

static const struct file_operations show_file_ops = {
	.read = read_dumps,
};

static const struct file_operations manual_log_ops = {
	.write = manual_log,
};

static const struct file_operations stop_file_ops = {
	.write = write_stop,
};

static int __init numa_dump_init(void)
{
	numa_dump_dir = debugfs_create_dir("numa_dump", NULL);
	if (!numa_dump_dir) {
		printk(KERN_ERR "Failed to create debugfs directory\n");
		return -ENODEV;
	}

	numa_dump_manual_log = debugfs_create_file(
		"manual_log", 0644, numa_dump_dir, NULL, &manual_log_ops);
	if (!numa_dump_manual_log) {
		printk(KERN_ERR "Failed to create debugfs file: manual_log");
		debugfs_remove_recursive(numa_dump_dir);
		return -ENODEV;
	}

	numa_dump_stop_file = debugfs_create_file("stop", 0644, numa_dump_dir,
						  NULL, &stop_file_ops);
	if (!numa_dump_stop_file) {
		printk(KERN_ERR "Failed to create debugfs file: reset");
		debugfs_remove(numa_dump_dir);
		return -ENODEV;
	}

	numa_dump_start_file = debugfs_create_file("start", 0644, numa_dump_dir,
						   NULL, &start_file_ops);
	if (!numa_dump_start_file) {
		printk(KERN_ERR "Failed to create debugfs file: start");
		debugfs_remove_recursive(numa_dump_dir);
		return -ENODEV;
	}

	numa_dump_show_log = debugfs_create_file(
		"show_log", 0644, numa_dump_dir, NULL, &show_file_ops);
	if (!numa_dump_start_file) {
		printk(KERN_ERR "Failed to create debugfs file: show_log");
		debugfs_remove_recursive(numa_dump_dir);
		return -ENODEV;
	}

	pr_info("Debugfs file created at /sys/kernel/debug/numa_dump");

	dump_numa_log_event = &log_event_numa;
	return 0;
}
module_init(numa_dump_init);

static void __exit numa_dump_exit(void)
{
	dump_numa_log_event = NULL;
	debugfs_remove_recursive(numa_dump_dir);
	free_dumps(dumps);
	pr_warn("Module unloaded");
}
module_exit(numa_dump_exit);
