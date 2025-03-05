#!/usr/bin/env bats
# File: student_tests.sh
# Create your unit tests suite in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Pipeline: echo hello | cat" {
    run ./dsh <<EOF
echo hello | cat
exit
EOF
    # Expect "hello" to appear in the output
    [[ "$output" == *"hello"* ]]
    [ "$status" -eq 0 ]
}

@test "Pipeline: echo hello world | tr a-z A-Z" {
    run ./dsh <<EOF
echo hello world | tr a-z A-Z
exit
EOF
    # Expect output to include "HELLO WORLD"
    [[ "$output" == *"HELLO WORLD"* ]]
    [ "$status" -eq 0 ]
}

@test "Pipeline: ls | grep \".c\"" {
    run ./dsh <<EOF
ls | grep ".c"
exit
EOF
    # Expect at least one .c file in the output (e.g. dshlib.c)
    [[ "$output" == *".c"* ]]
    [ "$status" -eq 0 ]
}

@test "Pipeline: multiple pipes: echo hello | tr a-z A-Z | rev" {
    run ./dsh <<EOF
echo hello | tr a-z A-Z | rev
exit
EOF
    # "hello" becomes "HELLO" then reversed "OLLEH"
    [[ "$output" == *"OLLEH"* ]]
    [ "$status" -eq 0 ]
}

@test "Built-in cd: change directory to /tmp and check pwd" {
    run ./dsh <<EOF
cd /tmp
pwd
exit
EOF
    # Expect output to contain "/tmp"
    [[ "$output" == *"/tmp"* ]]
    [ "$status" -eq 0 ]
}

@test "Built-in cd with no arguments does nothing" {
    current=$(pwd)
    run ./dsh <<EOF
cd
pwd
exit
EOF
    # Expect the current directory to remain unchanged
    [[ "$output" == *"$current"* ]]
    [ "$status" -eq 0 ]
}

@test "Empty input produces warning" {
    run ./dsh <<EOF
       
exit
EOF
    # Expect output to include the no-command warning
    [[ "$output" == *"warning: no commands provided"* ]]
    [ "$status" -eq 0 ]
}

@test "Invalid command returns error message" {
    run ./dsh <<EOF
nonexistentcommand
exit
EOF
    # Expect an error message indicating failure to execute external command
    [[ "$output" == *"error: could not run external command"* ]]
    [ "$status" -eq 0 ]
}

@test "Command with quoted spaces preserves them" {
    run ./dsh <<EOF
echo "hello,    world"
exit
EOF
    # Expect output to include the phrase with multiple spaces exactly as typed
    [[ "$output" == *"hello,    world"* ]]
    [ "$status" -eq 0 ]
}

# Extra Credit: Redirection tests (if implemented)

@test "Redirection: echo with output redirection" {
    run ./dsh <<EOF
echo "hello, redirection" > out.txt
cat out.txt
exit
EOF
    # Expect "hello, redirection" to appear in the file output
    [[ "$output" == *"hello, redirection"* ]]
    [ "$status" -eq 0 ]
}

@test "Extra Credit++: Append redirection" {
    run ./dsh <<EOF
echo "first line" > out.txt
echo "second line" >> out.txt
cat out.txt
exit
EOF
    # Expect both lines to appear in the correct order
    [[ "$output" == *"first line"* ]]
    [[ "$output" == *"second line"* ]]
    [ "$status" -eq 0 ]
}
