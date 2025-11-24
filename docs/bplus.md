# 📘 B+ Tree Usage in the Query Processing Engine

This document provides a brief overview of how the B+ tree data structure is used within the **Parallel Query Processing Engine**, its role in indexing, and the core functionality it enables.

---

## 🌳 What the B+ Tree Does

The B+ tree acts as the **primary indexing structure** for tables in the engine.
Its core job is to provide:

* **Fast lookup** of records based on a single attribute (the key)
* **Efficient range queries** (e.g., `WHERE id BETWEEN X AND Y`)
* **Sorted iteration** across leaf nodes
* **Stable performance** even as the dataset grows

Every key stored in the B+ tree maps to a **pointer to the actual table row** in memory.

---

## 🧱 Why We Use a B+ Tree

A B+ tree is ideal for a database-like system because:

### ✔ O(log n) Search & Insert

Tree height remains small even for large datasets, giving predictable query times.

### ✔ Great for Range Scans

Leaf nodes are linked in a chain, allowing fast sequential access without re-traversing the tree.

### ✔ Supports High Fanout

By adjusting the tree order, we keep the tree shallow and reduce pointer chasing.

### ✔ Matches Real Database Index Behavior

Postgres, MySQL, and almost all RDBMSes use B+ trees for default indexing.

---

## 🗂️ How It Fits Into the Engine Architecture

The engine is structured into three core components:

1. **bplus-serial.c**
   Implements the B+ tree storage structure itself.

2. **buildEngine-serial.c**
   Builds tables and creates B+ tree indexes over chosen attributes.

3. **queryEngine-serial.c (planned)**
   Executes commands (SELECT, WHERE) and uses the B+ tree for fast lookups.

### Index Lifecycle

* When a table is built, the engine creates one or more B+ tree indexes for selected attributes.
* During execution, a query such as:

  ```sql
  SELECT * FROM Users WHERE id = 42;
  ```

  does **not** scan a table linearly.
  Instead, the engine:

  1. Finds the B+ tree index for `id`
  2. Performs an O(log n) lookup
  3. Returns the pointer to the exact row

### Range Query Example

For:

```sql
SELECT * FROM Orders WHERE amount BETWEEN 50 AND 100;
```

The B+ tree:

1. Locates the leaf node containing the first key ≥ 50
2. Walks leaf-to-leaf until keys exceed 100
3. Yields row pointers in sorted order

This is **far faster** than scanning the entire table.

---

## 🔧 Functionality Summary

The current B+ tree implementation supports:

### **Searching**

* `find()` returns the row pointer matching a given key
* `findRange()` returns all rows in a given key interval

### **Insertion**

* Insertions automatically split nodes and rebalance the tree
* Existing keys are updated (upsert behavior)

### **Leaf-Level Linked List**

* Enables efficient sequential reads
* Powers range-based filtering

### **Configurable Key Type**

* The engine can specialize the B+ tree by adjusting `bpt_key_t` (e.g., integer, uint64_t)

---

## 🚀 Role in Future Parallel Processing

As we extend the system into a **parallel** query engine:

* B+ trees will be duplicated or partitioned across workers
* Range scans can be split across threads
* Indexes allow partition-aware scans rather than brute-force traversal
* Operations like distributed merge-joins can rely on sorted leaf chains

This makes the B+ tree a **central performance component** of the entire engine.

---

If you want, I can also create a matching diagram for this (ASCII or SVG), or help you write the README section for the full indexing subsystem.
