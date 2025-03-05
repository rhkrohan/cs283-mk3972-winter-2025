#!/usr/bin/env bats

# File: student_tests.sh
#
# These tests supplement the assignment_tests.sh file.
# They are run by `make test` alongside assignment_tests.sh.

###############################################################
# SINGLE COMMANDS
###############################################################

@test "Single command: echo hello" {
    run ./dsh <<EOF
echo hello
exit
EOF
    [[ "$output" =~ "hello" ]]
    [ "$status" -eq 0 ]
}

@test "Single command: ls (should succeed)" {
    run ./dsh <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Invalid command: nonexistent" {
    run ./dsh <<EOF
nonexistent_command_abc
exit
EOF
    [[ "$output" =~ "error: could not run external command" ]]
    [ "$status" -eq 0 ]
}

###############################################################
# BUILT-IN COMMANDS
###############################################################

@test "Built-in cd: no argument does nothing" {
    current_dir="$(pwd)"
    run ./dsh <<EOF
cd
pwd
exit
EOF
    [[ "$output" =~ "$current_dir" ]]
    [ "$status" -eq 0 ]
}

@test "Built-in cd: changes directory to /tmp" {
    run ./dsh <<EOF
cd /tmp
pwd
exit
EOF
    [[ "$output" =~ "/tmp" ]]
    [ "$status" -eq 0 ]
}

@test "Built-in exit: returns immediately" {
    run ./dsh <<EOF
exit
echo "SHOULD NOT SEE THIS"
EOF
    # We should NOT see "SHOULD NOT SEE THIS" in the output
    [[ ! "$output" =~ "SHOULD NOT SEE THIS" ]]
    [ "$status" -eq 0 ]
}

###############################################################
# QUOTED ARGUMENTS
###############################################################

@test "Quoted arguments: echo with double-quoted spaces" {
    run ./dsh <<EOF
echo " hello   world "
exit
EOF
    # Expect to see the entire string with spaces preserved
    [[ "$output" =~ " hello   world " ]]
    [ "$status" -eq 0 ]
}

@test "Quoted arguments: multiple quotes" {
    run ./dsh <<EOF
echo "one" "two" three
exit
EOF
    # Expect to see "one" "two" three as separate tokens
    [[ "$output" =~ "one two three" ]]
    [ "$status" -eq 0 ]
}

###############################################################
# PIPELINES
###############################################################

@test "Simple pipeline: echo hello | cat" {
    run ./dsh <<EOF
echo hello | cat
exit
EOF
    [[ "$output" =~ "hello" ]]
    [ "$status" -eq 0 ]
}

@test "Multiple pipeline stages: echo hello | tr a-z A-Z | rev" {
    run ./dsh <<EOF
echo hello | tr a-z A-Z | rev
exit
EOF
    # "hello" => "HELLO" => reversed => "OLLEH"
    [[ "$output" =~ "OLLEH" ]]
    [ "$status" -eq 0 ]
}

###############################################################
# EXTRA CREDIT: REDIRECTION (if implemented)
###############################################################

@test "Redirect output: echo -> file" {
    # Only meaningful if your shell implements redirection
    run ./dsh <<EOF
echo "test redirection" > test_output.txt
cat test_output.txt
exit
EOF
    [[ "$output" =~ "test redirection" ]]
    [ "$status" -eq 0 ]
}

@test "Append redirect: echo >> file" {
    # Only meaningful if your shell implements append
    run ./dsh <<EOF
echo "line1" > test_append.txt
echo "line2" >> test_append.txt
cat test_append.txt
exit
EOF
    [[ "$output" =~ "line1" ]]
    [[ "$output" =~ "line2" ]]
    [ "$status" -eq 0 ]
}
