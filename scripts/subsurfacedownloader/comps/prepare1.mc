<%class>
use Git::Repository;
my $downloader = "/home/robert/src/subsurface/build-downloader/subsurface-downloader";
my %dcs;
my $vendor;
my $product;
my $device;

&load_supported_dcs;

sub load_supported_dcs {
  open IN, "$downloader --list-dc|" || die "Cannot run $downloader: $!";
  
  while(<IN>) {
    last if /Supported dive computers:/;
  }
  while(<IN>) {
    last unless /\S/;

    my ($manufacturer, $products) = /"([^:]+):\s+([^"]+)"/;

    next unless defined $products;
    $products =~ s/\([^\)]*\)//g;
    my @products  = split /,\s*/, $products;
    $dcs{$manufacturer} = \@products;
    print STDERR "$manufacturer\n";
  }
  close IN;
}


1;
</%class>


<html>
	<head>
		<link rel="stylesheet" href="/static/css/style.css">
		<title>Subsurface Downloader</title>
	</head>

	<body>
		<select id="Manufacturer" onchange="loadProducts()">
% foreach $vendor (sort keys %dcs) {
  	 	       <option><% $vendor %>
% }
		</select>
		<select id="Product">
		</select>
 <script>
 function loadProducts() {
   var selected = document.getElementById("Manufacturer").value
   var vendorproducts = {
% foreach $vendor (sort keys %dcs) {
  	  "<% $vendor %>": "<option><% join "</option><option>",@{$dcs{$vendor}} %></option>",
%       }
  }
   document.getElementById("Product").innerHTML = vendorproducts[selected]
   }
   </script>


	</body>
</html>
