#!/usr/bin/env bats
# File: student_tests.sh
#

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

@test "Single command: echo with no arguments" {
    run ./dsh <<EOF
echo
exit
EOF
    # Depending on echo, output may be empty or just a newline.
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

@test "Single command: pwd" {
    run ./dsh <<EOF
pwd
exit
EOF
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

@test "Built-in cd: invalid directory produces error" {
    run ./dsh <<EOF
cd /nonexistent_directory
exit
EOF
    [[ "$output" =~ "cd:" ]] || [[ "$output" =~ "No such file" ]]
    [ "$status" -eq 0 ]
}

@test "Built-in cd: extra whitespace after cd" {
    # Even with extra spaces, the command should be recognized.
    run ./dsh <<EOF
cd      /
pwd
exit
EOF
    [[ "$output" =~ "^/$" ]] || [[ "$output" =~ "/" ]]
    [ "$status" -eq 0 ]
}

@test "Built-in exit: returns immediately" {
    run ./dsh <<EOF
exit
echo "SHOULD NOT SEE THIS"
EOF
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
    [[ "$output" =~ " hello   world " ]]
    [ "$status" -eq 0 ]
}

@test "Quoted arguments: multiple quotes" {
    run ./dsh <<EOF
echo "one" "two" three
exit
EOF
    [[ "$output" =~ "one two three" ]]
    [ "$status" -eq 0 ]
}

@test "Quoted arguments: complex mix of quoted and unquoted tokens" {
    run ./dsh <<EOF
echo mix "of quoted" and unquoted
exit
EOF
    [[ "$output" =~ "mix of quoted and unquoted" ]]
    [ "$status" -eq 0 ]
}

@test "Quoted arguments: missing closing quote" {
    run ./dsh <<EOF
echo "unclosed string
exit
EOF
    # Our parser treats the rest of the line as the argument.
    [[ "$output" =~ "unclosed string" ]]
    [ "$status" -eq 0 ]
}


@test "Quoted arguments: empty string" {
    run ./dsh <<EOF
echo ""
exit
EOF
    # Echoing an empty string might produce just a newline.
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
    [[ "$output" =~ "OLLEH" ]]
    [ "$status" -eq 0 ]
}

@test "Pipeline with extra spaces around the pipe" {
    run ./dsh <<EOF
echo hello   |    cat
exit
EOF
    [[ "$output" =~ "hello" ]]
    [ "$status" -eq 0 ]
}

@test "Pipeline with missing command between pipes" {
    run ./dsh <<EOF
echo hello |  | wc
exit
EOF
    # Even if one token is empty, the shell should handle it gracefully.
    [ "$status" -eq 0 ]
}

@test "Pipeline with trailing pipe" {
    run ./dsh <<EOF
echo hello |
exit
EOF
    # Should not crash; status 0 is expected even if pipeline is malformed.
    [ "$status" -eq 0 ]
}

###############################################################
# WHITESPACE AND EMPTY INPUT
###############################################################

@test "Empty command: just whitespace is ignored" {
    run ./dsh <<EOF
     
echo hello
exit
EOF
    [[ "$output" =~ "hello" ]]
    [ "$status" -eq 0 ]
}

@test "Multiple newlines: ignore empty commands" {
    run ./dsh <<EOF
echo first

echo second

exit
EOF
    [[ "$output" =~ "first" ]]
    [[ "$output" =~ "second" ]]
    [ "$status" -eq 0 ]
}

@test "Command with leading and trailing whitespace" {
    run ./dsh <<EOF
    echo trimmed    
exit
EOF
    [[ "$output" =~ "trimmed" ]]
    [ "$status" -eq 0 ]
}

@test "Multiple spaces between arguments" {
    run ./dsh <<EOF
echo  multiple   spaces
exit
EOF
    [[ "$output" =~ "multiple spaces" ]]
    [ "$status" -eq 0 ]
}

@test "Tab characters within arguments" {
    run ./dsh <<EOF
echo "hello	world"
exit
EOF
    # The output should contain a tab between hello and world.
    [[ "$output" =~ $'hello\tworld' ]]
    [ "$status" -eq 0 ]
}

###############################################################
# SEQUENTIAL COMMANDS
###############################################################

@test "Multiple commands in one session" {
    run ./dsh <<EOF
echo first
echo second
echo third
exit
EOF
    [[ "$output" =~ "first" ]]
    [[ "$output" =~ "second" ]]
    [[ "$output" =~ "third" ]]
    [ "$status" -eq 0 ]
}

@test "Exit command in middle stops execution" {
    run ./dsh <<EOF
echo before exit
exit
echo after exit
EOF
    [[ "$output" =~ "before exit" ]]
    [[ ! "$output" =~ "after exit" ]]
    [ "$status" -eq 0 ]
}

###############################################################
# REDIRECTION (EXTRA CREDIT IF IMPLEMENTED)
###############################################################

@test "Redirect output: echo -> file" {
    run ./dsh <<EOF
echo "test redirection" > test_output.txt
cat test_output.txt
exit
EOF
    [[ "$output" =~ "test redirection" ]]
    [ "$status" -eq 0 ]
}

@test "Append redirect: echo >> file" {
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

###############################################################
# EDGE CASES & MALFORMED INPUT
###############################################################

@test "Whitespace-only input lines" {
    run ./dsh <<EOF
     
      
echo nonempty
     
exit
EOF
    [[ "$output" =~ "nonempty" ]]
    [ "$status" -eq 0 ]
}
