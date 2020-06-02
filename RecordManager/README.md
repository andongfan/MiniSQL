## Gibberish

## Makefile

放在外层目录，和testrm.cpp放在一起。

```
cmake_minimum_required(VERSION 3.5)

project(rmtest)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(API)
add_subdirectory(CatalogManager)
add_subdirectory(Interpreter)
add_subdirectory(BufferManager)
add_subdirectory(RecordManager)

add_executable(${PROJECT_NAME}
    testrm.cpp
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC catalog_manager interpreter API buffer_manager record_manager
)
```

## Questions

1. do I need to check the correctness of insert/record attributes?

## File Organization

每个表存储在一个文件中

表的元数据来自 API 输入的 Table 对象。

定长存储，因此每个 block 可存储的记录个数恒定，从 block 开头开始存储，最后可能多出一些废弃的字节。

一个记录在文件中的存储比记录的原长度多 1，第一个字节的代表当前位置是否已经被占用如下：

## Functions

### CreateTable

input: Table class object
output: pageID to the 
creates a table (takes catalog manager as input, use buffer manager to create file)

### DropTable

input: table class object

### InsertRecord

input: table class object, vector of attributes
insert the record into the table
need to check uniqueness

### DeleteRecord

input: table class object, primary key
delete one record

maybe make the table compact by compressing from time to time?

(when doing conditional deletion, run select and delete them one by one)

### SelectRecord

input: table class object, list of Conditions
select the records that satisfy the condition
bruteforce?