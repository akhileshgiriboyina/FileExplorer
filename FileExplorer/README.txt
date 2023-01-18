Upon pressing ':', terminal enters into command mode where-in user can enter commands to perform actions like Create/Move/Search Files or Directories

List of commands with Syntax
- COPY File/Directory to destination path
  copy <source_file(s)> <destination_directory>

- MOVE File/Directory to destination path
  move <source_file(s)> <destination_directory>

- RENAME a File or Directory to a new name under working directory
  rename <old_filename> <new_filename>

- CREATE file
  create_file <file_name> <destination_path>

- CREATE Directory
  create_dir <dir_name> <destination_path>

- GOTO (Set different working directory and diasplay files/folders in it)
  goto <location>

- SEARCH for a file or directory recursively under working directory
  search <file_name/directory_name>


Note:
Input-Path/File should be given within quotation ("").
Ex:
rename "AOS 1" "AOS"
create_file "123 abc.txt" "~/Work/IIIT Hyderabad/Files/"
