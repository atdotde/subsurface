#!/usr/bin/perl

use strict;
use CGI;
use Mail::SendEasy;


my $q = CGI::new;

if (scalar $q->param('report')) {
        print "Content-type: text/html\n\n";
	my $mail = Mail::SendEasy->new(smtp => 'localhost',
	                               # user => 'SMTPUSER',
				       # pass => 'SMTPPASSWORD'
				       );

        my $status = $mail->send(
	                         # from => 'bugreporter@subsurface-divelog.org' ,
				 from => 'obert@thetheoreticaldiver.org' ,
				 from_title => 'Mobile App Bug Reporter' ,
				 to => 'robert@thetheoreticaldiver.org',
				 cc => $q->param('email') || '',
				 subject => "Subsurface-mobile Bug Report",
				 msg => $q->param('report'),
				 msgid => "SiubusrfaceBug-".$q->param('email').'-1'
				) ;

        if (!$status) {
	        print $mail->error;
	} else {
	        print "OK\n";
	}
}
