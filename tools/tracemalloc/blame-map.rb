#!/usr/bin/ruby

#
# This is a one-off script for converting a TraceMalloc snapshot 
# into a treemap.  Approach it with low expectations, and be prepared
# to make changes.
#
# The output from this script works with the Javascript JIT viewer.
# See thejit.org, treemap.html, and 
# http://wiki.getnightingale.com/doku.php?id=developer_center:articles:debugging:using_tracemalloc
#

require 'rubygems'
require 'json'
require File.dirname(__FILE__) + '/tracemalloc'


#
# Remap classes, functions, or libraries to arbitrary names,
# ignore entirely, or break apart to show classes. 
#
# If you want more details for a specific library, map it
# to :breakapart. If you want to reduce noise from a group
# of libraries, map them to a common name.
#
# This is intended as a lightweight version of histogram.pl's
# type inference system.  It answers the same questions, but
# involves way less manual effort.
#
# This list is OSX specific. Make your own if you want to run
# on another platform.
#
$blame_list = {
  
  "my_malloc_logger" => "",
  "malloc_zone_malloc" => "",
  "malloc" => "",
  "operator new(unsigned long)" => "",
  "libobjc.A.dylib" => :ignore,
  "libstdc++.6.dylib" => "",
  "libSystem.B.dylib" => "",
  "PR_Malloc" => "",
#  "(null)" => :ignore,

  # Apple-y stuff.  Maybe break this in half. 
  "AppKit" => "Apple",
  "CoreFoundation" => "Apple",
  "Foundation" => "Apple",
  "HIServices" => "Apple",
  "HIToolbox" => "Apple",
  "CoreGraphics" => "Apple",
  "CoreText" => "Apple",
  "LaunchServices" => "Apple",
  "CoreUI" => "Apple",
  "ATS" => "Apple",
  "CarbonCore" => "Apple",
  "libCSync.A.dylib" => "Apple",
  "ColorSync" => "Apple",
  "libRIP.A.dylib" => "Apple",
  "libCGATS.A.dylib" => "Apple",
  "ImageIO" => "Apple",
  "QuartzCore" => "Apple",
  "SIMBL" => "Apple",
  "QD" => "Apple",
  "Shortcut" => "Apple",
  
  "songbird" => :ignore,
  "XRE_main" => :ignore,

  # "sbDBEngine.dylib" => :breakapart,
  # "sbLocalDatabaseLibrary.dylib" => :breakapart,
  # "sbMetadataModule.dylib" => :breakapart,  

  # libxul objects of interest
  "nsXBLBinding" => "XBL",
  "nsXULElement" => "nsXULElement",
  "nsXBLBinding" => "XBL",
  "nsXBLPrototypeBinding" => "XBL",
  "nsXBLService" => "XBL",
  "nsCSSLoaderImpl" => "nsCSSLoaderImpl",
  "CSSParserImpl" => "CSSParserImpl",
  "nsParser" => "nsParser",
  "nsStringBuffer" => "nsStringBuffer",
  "nsStyleSet" => "nsStyleSet",
  "nsXULDocument" => "nsXULDocument",
  "nsStringBundle" => "nsStringBundle",
  "nsStandardURL" => "nsStandardURL",
  "mozJSComponentLoader" => "mozJSComponentLoader",
  "mozStorageConnection" => "mozStorageConnection",
  "nsNavHistory" => "nsNavHistory",
  "mozJSSubScriptLoader" => "mozJSSubScriptLoader",
  "nsPrefBranch" => "nsPrefBranch",
  "nsTHashtable" => "nsTHashtable",
  "mozStorageStatement" => "mozStorageStatement",
  "nsJARChannel" => "nsJARChannel",
  "nsHttpChannel" => "nsHttpChannel",

#  "XUL" => "Misc XUL",
  "libnspr4.dylib" => "NSPR",
  "libplds4.dylib" => "PLDS",
  "libmozjs.dylib" => "Javascript",
}

#
#  Names to ignore in build_flat_blame_tree
#
$ignore_list = {
  "my_malloc_logger" => 1,
  "malloc_zone_malloc" => 1,
  "malloc" => 1,
  "operator new(unsigned long)" => 1,
  "libobjc.A.dylib" => 1,
  "AppKit" => 1,
  "XUL" => 1,
  "libnspr4.dylib" => 1,
  "CoreFoundation" => 1,
  "libstdc++.6.dylib" => 1,
  "Foundation" => 1,
  "libplds4.dylib" => 1,
  "HIServices" => 1,
  "HIToolbox" => 1,
  "CoreGraphics" => 1,
  "CoreText" => 1,
  "LaunchServices" => 1,
  "CoreUI" => 1,
  "ATS" => 1,
  "songbird" => 1,
  "CarbonCore" => 1,
  "libSystem.B.dylib" => 1
}



#
# Use the blame list to come up with a text label
# for the given stack frame
#
def get_label_for_frame(frame) 
  # Reduce noise by only looking at the object for C++
  # and name for libraries 
  if frame[:library] =~ /.*\/(.*)/
    frame[:library] = $1
  end
  library = frame[:library]
  call = frame[:function].scan(/(\w*)(\:\:)?.*/)[0][0]

  new_label = nil

  # First look up the library 
  if $blame_list.include?(library) 
    new_label = $blame_list[library]
    
    # Explicitly show calls if requested
    if new_label == :breakapart 
      if $blame_list.include?(call) 
        new_label = $blame_list[call]
      else 
        new_label = call
      end
    end
  elsif $blame_list.include?(call) 
    # If a label has been given for the call, used that
    new_label = $blame_list[call]
  else 
    # Otherwise group by library name in order to 
    # avoid being flooded with data
    new_label = library
  end
  return new_label
end



# 
# Build a tree showing how much each library
# allocated, ignoring and merging frames according
# to blame_list
#
def build_blame_tree  
  tree = { :value => 0, :children => {} }

  # Read tracemalloc snapshot from stdin
  TraceMalloc.read_snapshot { |allocation| 
  
    # Walk the stack, blaming the allocation on each label found
  
    tree[:value] += allocation[:size]
    node = tree[:children]
  
    label = nil
    new_label = nil
  
    allocation[:stack].reverse_each { |frame| 
      new_label = get_label_for_frame(frame)
      
      # Skip frames that have been explicitly ignored
      # and collapse multiple frames with the same label
      next unless new_label != :ignore and new_label != label
                  
      label = new_label
    
      if not node.include?(label)
        node[label] = { :value => 0, :children => {} } 
      end

      node[label][:value] += allocation[:size]
      node[label][:library] = frame[:library]

      node = node[label][:children]
    }
  }
  
  return tree
end


# 
# Experimental function to charge allocations
# to callers and overcount, so that everything
# is a top level node (no children)
#  To test, run
#    print "var json = ", build_json_treemap(build_flat_blame_tree, nil).to_json, ";"
#
def build_flat_blame_tree  
  tree = { :value => 0, :children => {} }

  # Read tracemalloc snapshot from stdin
  TraceMalloc.read_snapshot { |allocation| 
  
    # Walk the stack, blaming the allocation on each library found
  
    tree[:value] += allocation[:size]
    node = tree[:children]
  
    library = nil
  
    allocation[:stack].reverse_each { |frame| 
      next unless library != frame[:library]
      next if $ignore_list.include?(frame[:library]) or
              $ignore_list.include?(frame[:function])
              
      library = frame[:library]
    
      if not node.include?(library)
        node[library] = { :value => 0, :children => {} } 
      end

      node[library][:value] += allocation[:size]
    }
  }
  
  return tree
end



#
# JSON
#

$id_counter = 0

def build_json_treemap(real_node, json_node) 
  return if not real_node
  
  if not json_node
    json_node = { 
      "id" => "root", 
      "name" => "root", 
      "data" => [{ "key" => "Size", "value" => real_node[:value] },
                 { "key" => "Library", "value" => real_node[:library] }],
      "children" => []
    }
  end
  
  real_node[:children].each { |name, data|
    $id_counter = $id_counter + 1
    id = name + $id_counter.to_s
    new_node = { 
      "id" => id, 
      "name" => name, 
      "data" => [{ "key" => "Size", "value" => data[:value] },
                 { "key" => "Library", "value" => data[:library] }],
      "children" => []
    }    
    build_json_treemap(data, new_node)
    json_node["children"].push(new_node)
  }
  
  return json_node
end


print "var json = ", build_json_treemap(build_blame_tree, nil).to_json, ";"

