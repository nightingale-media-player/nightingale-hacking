#!/usr/bin/ruby

#
# Utility functions for parsing and manipulating TraceMalloc
# Snapshot output.  
#
# See http://www.mozilla.org/projects/footprint/live-bloat.html
#

module TraceMalloc

  #
  # Read basic tracemalloc snapshot, yielding a record at a time
  #
  def self.read_snapshot
    while gets do 
      # Match start of an allocation record like "0x0832FBD0 <void*> (80)"
      next unless $_ =~ /^0x(\S+) <(.*)> \((\d+)\)/;
      allocation = { :type => $2, :size => $3.to_i }

      # Skip slot data (like "\t0x00000000"), since it isn't used
      while gets and $_ =~ /^\t0x(\S+)/ do end

      # Build stack from records like 
      #   "_dl_debug_message[/lib/ld-linux.so.2 +0x0000B858]"
      stack = []
      while gets and $_ =~ /^(.*)\[(.*) \+0x(\S+)\]$/ do
        stack.push({ :function => $1, :library => $2, :offset => $3 })
      end
    
      allocation[:stack] = stack

      yield allocation
    end
  end

  #
  # Read just the address/type/size lines from a tracemalloc snapshot, 
  # yielding one at a time
  #
  def self.read_snapshot_signatures(file)
    while content = file.gets do 
        # Match start of an allocation record like "0x0832FBD0 <void*> (80)"
        next unless content =~ /^0x(\S+) <(.*)> \((\d+)\)/;
        yield content
    end
  end

end