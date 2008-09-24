#!/usr/bin/ruby

#
# Description:
#  This is a one-off script intended to dump the output from 
#  run.sh into a csv format suitable for scatter plotting
#  with Apple's Numbers program.  You'll probably 
#  need to alter the format if you want to use it with Excel.
#
# Usage:
#  ./make-dbperf-spreadsheet.rb < dbperf.log > dbperf.csv
#

# Read the log data from stdin
sizes = {}
tests = {}

$stdin.each do |line|
    # Expects log files in the form "testname\tdbname\tlibrarysize\ttime"
    name, library, size, time = line.split("\t")
    
    # Map test names to an array of test results for each test size
    tests[name] = {} if not tests.include?(name);
    tests[name][size] = [] if not tests[name].include?(size);
    tests[name][size].push(time.chomp);
    
    # Track how many samples we have at each library size
    sizes[size] = tests[name][size].length if !sizes[size] or tests[name][size].length > sizes[size];
end

# Write the column headers 
print tests.keys.join(",Library Size,"), ",Library Size,\n"

# Now write out the data  
# (Put a size column beside each test column, since Numbers is picky)
sizes.keys.sort{|a,b| a.to_i <=> b.to_i}.each do |size| 
  for i in 0..(sizes[size] - 1)
    tests.keys.each do |test|
      print size, ",", tests[test][size][i], ","
    end
    print "\n"  
  end
end


