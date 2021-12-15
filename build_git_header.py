import subprocess

header_file_name = "git_header.h"

def get_git_revision_hash():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('ascii').strip()

def get_git_revision_short_hash():
    return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()

def get_git_working_tree_clean():
	if len(subprocess.check_output(['git', 'status', '-s'])) == 0:
		return "1"
	return "0"

def get_git_revision_hash_from_header_file():
	try:
		header = open(header_file_name, "r")
		words = header.read().split()
		return words[words.index("GIT_HASH") + 1].strip('\"')
	except:
		return ""

def get_git_working_tree_clean_from_header_file():
	try:
		header = open(header_file_name, "r")
		words = header.read().split()
		return words[words.index("GIT_WORKING_TREE_CLEAN") + 1]
	except:
		return ""

def write_header_file():
	header = open(header_file_name, "w")
	header.write('''// Automatically generated file by ''' + __file__ + ''' 

#ifndef HAVE_VERSION_H
#define HAVE_VERSION_H

#define GIT_HASH "''' + get_git_revision_hash() + '''"
#define GIT_SHORT_HASH "''' + get_git_revision_short_hash() + '''"
#define GIT_WORKING_TREE_CLEAN ''' + get_git_working_tree_clean() + '''
#define GIT_WORKING_TREE "''' + ( "clean" if get_git_working_tree_clean() == "1" else "modified" ) + '''"
#define GIT_SHORT_HASH_WITH_MODIFIED "''' + get_git_revision_short_hash() + ( " modified" if get_git_working_tree_clean() != "1" else "") + '''" 

#endif // HAVE_VERSION_H
'''
	)
	header.close()

def main():
	if (get_git_revision_hash() != get_git_revision_hash_from_header_file()) or (get_git_working_tree_clean() != get_git_working_tree_clean_from_header_file()):
		write_header_file()

if __name__ == "__main__":
	main()
