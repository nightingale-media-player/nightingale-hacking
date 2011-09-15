#!/usr/bin/ruby

#
# Given two TraceMalloc snapshots, produces a new snapshot that
# includes only allocations which appeared between the original snapshots.
#
# Usage: ./whatsnew.rb before.snapshot after.snapshot > newstuff.snapshot
#

require File.dirname(__FILE__) + '/tracemalloc'

# Collect all allocation signatures from the first file
signatures = {}
file = File.open(ARGV[0])
TraceMalloc.read_snapshot_signatures(file) { |signature|
  signatures[signature] = true
}
file.close

# Now read through the second file, emitting complete records 
# for all allocations not found in the first file
file = File.open(ARGV[1])
is_new = false
while line = file.gets do 
  # Match start of an allocation record like "0x0832FBD0 <void*> (80)"
  if line =~ /^0x(\S+) <(.*)> \((\d+)\)/
    is_new = signatures.include?(line) == false
  end
  if is_new
    print line
  end
end
file.close