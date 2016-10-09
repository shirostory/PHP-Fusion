<?php
if (!defined("IN_FUSION")) {
	die("Access Denied");
}

class MemberPoll {
    private static $instance = NULL;
    private static $rows = 0;
    private static $locale = array();
    private static $opt_count = 2;
    private static $limit = 4;

    private $data = array(
	'poll_id' 		=> 0,
    'poll_title' 	=> '',
    'poll_opt' 		=> array('',''),
    'poll_started' 	=> '',
    'poll_ended' 	=> '',
    'poll_visibility' => '',
	);

    public function __construct() {
        $this->set_locale();
        $_GET['action'] = isset($_GET['action']) ? $_GET['action'] : '';

        switch ($_GET['action']) {
            case 'delete':
        	self::delete_poll($_GET['poll_id']);
                break;
            case 'poll_add':
        	self::start_poll($_GET['poll_id']);
                break;
            case 'poll_lock':
        	self::poll_lock($_GET['poll_id']);
                break;
            case 'poll_unlock':
        	self::poll_unlock($_GET['poll_id']);
                break;
            default:
                 break;
        }
	add_breadcrumb(array('link' => '', 'title' => self::$locale['POLL_000']));
        self::set_polldb();
		if (defined('ADMIN_PANEL')){
			add_to_title(self::$locale['POLL_000']);
        	self::set_admin_polldb();
		}
    }

    private function set_admin_polldb() {
		if (isset($_POST['save'])) {
		$i = 0;
		while ($i <$_POST['opt_count']) {

		    foreach ($_POST['poll_opt_'.$i] as $key => $value) {
 		       if ($value != '') {
 		       $poll_opt[$i][$key] = $value;
		        }
		    }
	    $i++;
		}

    $poll_option = array_filter($poll_opt);
    $this->data = array(
    'poll_id' => !empty($_GET['poll_id']) ? form_sanitizer($_GET['poll_id'], 0, 'poll_id') : 0,
    'poll_title' => form_sanitizer($_POST['poll_title'], '', 'poll_title', TRUE),
    'poll_opt' => serialize($poll_option),
    'poll_visibility' => form_sanitizer($_POST['poll_visibility'], 0, 'poll_visibility') ,
    'poll_started' => form_sanitizer($_POST['poll_started'], 0, 'poll_started'),
    'poll_ended' => (isset($_POST['poll_ended']) ? form_sanitizer($_POST['poll_ended'], 0, 'poll_ended') : 0)
    );

        if (\defender::safe()) {
			addNotice("success", $this->data['poll_id'] == 0 ? self::$locale['POLL_005'] : self::$locale['POLL_006']);
    		dbquery_insert(DB_POLLS, $this->data, ($this->data['poll_id'] == 0 ? "save" : "update"));
    		redirect(clean_request("", array("section=poll", "aid"), TRUE));
        	}
		}
    }

    public static function getInstance($key = TRUE) {
        if (self::$instance === NULL) {
            self::$instance = new static();
        }

        return self::$instance;
    }

    private static function set_locale() {
        self::$locale = fusion_get_locale("", INFUSIONS."member_poll_panel/locale/".LOCALESET."poll.php");
    }

    public function _selectFormPoll($id) {

	$result = dbquery("SELECT poll_id, poll_title, poll_opt, poll_started, poll_ended, poll_visibility
	FROM ".DB_POLLS."
	WHERE poll_id='".intval($id)."'
	");
        if (dbrows($result) > 0) {
            $list = dbarray($result);
        }
	return $list;

    }

    public function _selectPoll() {
	$result = dbquery("SELECT poll_id, poll_title, poll_opt, poll_started, poll_ended, poll_visibility
	FROM ".DB_POLLS."
	WHERE ".groupaccess('poll_visibility')."
	ORDER BY poll_started DESC");
		if (dbrows($result)) {
			while ($data = dbarray($result)) {

			$pdata[] = $data;
			}
		return $pdata;
		}

    }

    public function _selectVote($user, $pollid) {

    $whr = iMEMBER ? "vote_user='".$user."' AND vote_user_ip='".USER_IP."'" : "vote_user_ip='".USER_IP."'";
	$result = dbquery("SELECT vote_id, vote_user, vote_opt, vote_user_ip, poll_id
	FROM ".DB_POLL_VOTES."
	WHERE poll_id='".$pollid."' AND ".$whr);
		if (!dbrows($result)) {
		return FALSE;
		} else {
		return TRUE;
		}
    }


    public function _selectDB($rows) {
	$result = dbquery("SELECT poll_id, poll_title, poll_opt, poll_started, poll_ended, poll_visibility
		FROM ".DB_POLLS."
		WHERE ".groupaccess('poll_visibility')."
		ORDER BY poll_id DESC
		LIMIT ".intval($rows).", ".self::$limit);
        return $result;
    }


    static function verify_poll($id) {
        if (isnum($id)) {
            return dbcount("(poll_id)", DB_POLLS, "poll_id='".intval($id)."'");
        }

        return FALSE;
    }

    private static function delete_poll($id) {
        if (self::verify_poll($id)) {
            dbquery("DELETE FROM ".DB_POLLS." WHERE poll_id='".intval($id)."'");
            addNotice('warning', self::$locale['POLL_007']);
	        redirect(clean_request("", array("section=poll", "aid"), TRUE));
        }
    }

    private static function start_poll($id) {
        if (self::verify_poll($id)) {
			dbquery("UPDATE ".DB_POLLS." SET poll_started='".time()."' WHERE poll_id='".intval($id)."'");
            addNotice('warning', self::$locale['POLL_008']);
	        redirect(clean_request("", array("section=poll", "aid"), TRUE));
        }
    }

    private static function poll_lock($id) {
        if (self::verify_poll($id)) {
			dbquery("UPDATE ".DB_POLLS." SET poll_ended='".time()."' WHERE poll_id='".intval($id)."'");

            addNotice('warning', self::$locale['POLL_009']);
	        redirect(clean_request("", array("section=poll", "aid"), TRUE));
        }
    }

    private static function poll_unlock($id) {
        if (self::verify_poll($id)) {
			dbquery("UPDATE ".DB_POLLS." SET poll_ended='0' WHERE poll_id='".intval($id)."'");

            addNotice('warning', self::$locale['POLL_010']);
	        redirect(clean_request("", array("section=poll", "aid"), TRUE));
        }
    }

    public function _countVote($opt) {
        $poll_votes = dbcount("(vote_id)", DB_POLL_VOTES, $opt);
    	return $poll_votes;
    }

    public function _countPoll($opt) {
        $poll = dbcount("(poll_id)", DB_POLLS, groupaccess('poll_visibility').$opt);
    	return $poll;
    }

    public function _renderpollOption() {

        if ($this->_selectPoll()) {

			foreach ($this->_selectPoll() as $id => $dat) {
        	$poll_opt = unserialize($dat['poll_opt']);
			$data[$id] = array(
				'poll_id' => $dat['poll_id'],
				'poll_started' => $dat['poll_started'],
				'poll_ended' => $dat['poll_ended'],
				'poll_visibility' => $dat['poll_visibility'],
				);
				for ($i = 0; $i < count($poll_opt); $i++) {
        			$data[$id]['poll_title'] = unserialize($dat['poll_title']);
					$data[$id]['poll_option'][$i] = !empty($poll_opt[$i][LANGUAGE]) ? $poll_opt[$i][LANGUAGE] : "";
				}
			}
   			return $data;
        } else {
        	return FALSE;
        }
    }

    public function _selectUserVote() {

	$data = $this->_renderpollOption();
	$info['vote'] = $this->_countPoll("");
    $info_f = array();
    $info_t = array();

	if (!empty($data)){
    $i = 0;
    $ii = 0;
		foreach ($data as $key => $dat) {

        $data_user = checkgroup($dat['poll_visibility']) && !empty($dat['poll_title'][LANGUAGE]) ?
        	$this->_selectVote(fusion_get_userdata((iMEMBER ? 'user_id' : 'user_ip')), $dat['poll_id']) :
        	TRUE;

		if ($data_user == FALSE) {
			if ($i == 0) {

		$info_f['poll_table'][$key]['max_vote'] = $this->_countVote("poll_id='".$dat['poll_id']."'");
		$info_f['poll_table'][$key]['poll_title'] = $dat['poll_title'][LANGUAGE];
        $form_action = FUSION_SELF.(FUSION_QUERY ? "?".FUSION_QUERY : "");

		$info_f['poll_table'][$key]['poll_option'][] = openform('voteform', 'post', $form_action, array('enctype' => TRUE));
		$info_f['poll_table'][$key]['poll_option'][] = form_hidden('poll_id', '', $dat['poll_id']);
			foreach ($dat['poll_option'] as $im1 => $data1) {
			$num_votes = $this->_countVote("vote_opt='".$im1."' AND poll_id='".$dat['poll_id']."'");
    		$opt_votes = ($num_votes ? number_format(($num_votes / $info_f['poll_table'][$key]['max_vote']) * 100, 0) : number_format(0 * 100, 0));
        	$info_f['poll_table'][$key]['poll_option'][] = "<label><input type='radio' name='check' value='".$im1."' /> ".$data1."</label>\n";
			}
		$info_f['poll_table'][$key]['poll_foot'][] = form_button("cast_vote", self::$locale['POLL_020'], self::$locale['POLL_020'], array('class' => 'btn-primary m-r-10'));
		$info_f['poll_table'][$key]['poll_foot'][] = closeform();
           $i++;
           }

		} else {
			if ($ii == 0) {
				if (!empty($dat['poll_title'][LANGUAGE]) && $dat['poll_started'] < time()){
				$info_t['poll_table'][$key]['max_vote'] = $this->_countVote("poll_id='".$dat['poll_id']."'");
				$info_t['poll_table'][$key]['poll_title'] = $dat['poll_title'][LANGUAGE];
				foreach ($dat['poll_option'] as $im1 => $data1) {
					$num_votes = $this->_countVote("vote_opt='".$im1."' AND poll_id='".$dat['poll_id']."'");
    				$opt_votes = ($num_votes ? number_format(($num_votes / $info_t['poll_table'][$key]['max_vote']) * 100, 0) : number_format(0 * 100, 0));

    				$info_t['poll_table'][$key]['poll_option'][] = progress_bar($opt_votes,  $data1,
                    	      FALSE, // class
                    	      FALSE,  // height
                    	      FALSE,  // reverse
                    	      TRUE,  // as percent
                    	      FALSE
                     	   );
				$info_t['poll_table'][$key]['poll_option'][] =  $opt_votes."% [".(empty($num_votes) ? 0 : $num_votes).($num_votes == 1 ? self::$locale['POLL_040'] : self::$locale['POLL_041'])."]";

    			}
        		$info_t['poll_table'][$key]['poll_foot'][] = self::$locale['POLL_060'].$info_t['poll_table'][$key]['max_vote'];
        		$info_t['poll_table'][$key]['poll_foot'][] = self::$locale['POLL_061'].showdate("shortdate", $dat['poll_started']);
        		if ($dat['poll_started'] < time() && (!empty($dat['poll_ended']) && ($dat['poll_ended'] < time()))) {
        			$info_t['poll_table'][$key]['poll_foot'][] = self::$locale['POLL_062'].showdate("shortdate", $dat['poll_ended']);
				}
           		$ii++;
           		}
            }
		}
		$render = empty($info_f) ? $info_t : $info_f;
		$render['poll_tablename'] = self::$locale['POLL_001'];
	        if ($info['vote'] > 1) {
	        $render['poll_arch'] = "<a class='btn btn-default btn-sm' href='".INFUSIONS."member_poll_panel/polls_archive.php'>".self::$locale['POLL_063']."</a>";
	        }
		}

		render_poll($render);
		}
    }

    public function display_poll() {
        $this->_selectUserVote();
    }

    public function archive_poll() {
        $this->_archive_poll();
    }

    public function display_admin() {
	$aidlink = fusion_get_aidlink();

	$allowed_section = array("poll", "poll_vote");
	$_GET['section'] = isset($_GET['section']) && in_array($_GET['section'], $allowed_section) ? $_GET['section'] : 'poll';
	$edit = (isset($_GET['action']) && $_GET['action'] == 'edit') && isset($_GET['poll_id']) ? TRUE : FALSE;
	$_GET['poll_id'] = isset($_GET['poll_id']) && isnum($_GET['poll_id']) ? $_GET['poll_id'] : 0;

	opentable(self::$locale['POLL_000']);
    $master_tab_title['title'][] = self::$locale['POLL_000'];
	$master_tab_title['id'][] = "poll";
	$master_tab_title['icon'][] = "";

	$master_tab_title['title'][] = $edit ? self::$locale['POLL_042'] : self::$locale['POLL_043'];
	$master_tab_title['id'][] = "poll_vote";
	$master_tab_title['icon'][] = "";

		echo opentab($master_tab_title, $_GET['section'], "poll", TRUE);
		switch ($_GET['section']) {
 		   case "poll_vote":
			add_breadcrumb(array('link' => FUSION_SELF.$aidlink, 'title' => $master_tab_title['title'][1]));
	        $this->poll_form();
	        break;
	    default:
			add_breadcrumb(array('link' => FUSION_SELF.$aidlink, 'title' => $master_tab_title['title'][0]));
	        $this->poll_listing();
	        break;
		}
		echo closetab();
	closetable();

    }

    public function poll_listing() {
	$aidlink = fusion_get_aidlink();

    $total_rows = $this->_countPoll("");

    $rowstart = isset($_GET['rowstart']) && ($_GET['rowstart'] <= $total_rows) ? $_GET['rowstart'] : 0;

	$result = $this->_selectDB($rowstart);

    $rows = dbrows($result);

    echo "<div class='clearfix'>\n";
    echo "<span class='pull-right m-t-10'>".sprintf(self::$locale['POLL_011'], $rows, $total_rows)."</span>\n";
    echo "</div>\n";

	echo ($total_rows > $rows) ? makepagenav($rowstart, self::$limit, $total_rows, self::$limit, clean_request("", array("aid", "section"), TRUE)."&amp;") : "";


	if ($rows > 0) {
	echo "<div class='row m-t-20'>\n";
        while ($data = dbarray($result)) {
		$title = unserialize($data['poll_title']);
		$poll_opt = unserialize($data['poll_opt']);
		echo "<div class='col-xs-12 col-sm-3'>\n";
		echo "<p class='display-inline-block badge'>".self::$locale['POLL_044'].getgroupname($data['poll_visibility'])."</p>\n";

			echo "<div class='panel panel-default'>\n";
			echo "<div class='panel-heading text-left'>\n";
			foreach ($title as $key => $info) {
				echo "<p>".(!empty($info) ? translate_lang_names($key).": ".$info : $info)."</p>\n";
			}

			echo "</div>\n";

			echo "<div class='overflow-hide' >\n";

			$db_info = dbcount("(vote_opt)", DB_POLL_VOTES, "poll_id='".$data['poll_id']."'");
			foreach ($poll_opt as $keys => $data1) {
				$text = "";
				foreach ($data1 as $key => $inf) {
					$text .= "<p>".(!empty($inf) ? translate_lang_names($key).": ".$inf : $inf)."</p>\n";
					}

				$num_votes = dbcount("(vote_opt)", DB_POLL_VOTES, "vote_opt='".$keys."' AND poll_id='".$data['poll_id']."'");
    			$opt_votes = ($num_votes ? number_format(($num_votes / $db_info) * 100, 0) : number_format(0 * 100, 0));
				echo progress_bar($opt_votes,  $text,
									FALSE, // class
									FALSE,  // height
									FALSE,  // reverse
									TRUE,  // as percent
									FALSE
							);
				echo "<p><strong>".$opt_votes."% [".(empty($num_votes) ? 0 : $num_votes).($num_votes == 1 ? self::$locale['POLL_040'] : self::$locale['POLL_041'])."]</strong></p>\n";

				}
				echo "<p><strong>".self::$locale['POLL_060'].$db_info."</strong></p>\n";
				echo "<p>".self::$locale['POLL_061'].showdate("shortdate", $data['poll_started'])."</p>\n";
				echo "<p class='display-inline-block badge'>".self::$locale['POLL_064'].($data['poll_started'] > time() ? self::$locale['POLL_065'] : (!empty($data['poll_ended']) && ($data['poll_ended'] < time()) ? self::$locale['POLL_066'] : self::$locale['POLL_067']))."</p>\n";
					if (!empty($data['poll_ended']) && $data['poll_ended'] < time()) {
					echo "<p>".self::$locale['POLL_068'].showdate("shortdate", $data['poll_ended'])."</p>\n";
					}
				echo "</div>\n";

				echo "<div class='panel-body'>\n";
				echo "<div class='dropdown'>\n";
				echo "<button data-toggle='dropdown' class='btn btn-default dropdown-toggle btn-block' type='button'>".self::$locale['POLL_021']."<span class='caret'></span></button>\n";
				echo "<ul class='dropdown-menu'>\n";
				echo "<li><a href='".FUSION_SELF.$aidlink."&amp;section=poll_vote&amp;action=edit&amp;poll_id=".$data['poll_id']."'><i class='fa fa-edit fa-fw'></i> ".self::$locale['edit']."</a></li>\n";
				echo "<li class='divider'></li>\n";
				if ($data['poll_started'] > time()){
				echo "<li><a href='".FUSION_SELF.$aidlink."&amp;section=poll&amp;action=poll_add&amp;poll_id=".$data['poll_id']."'><i class='fa fa-trash fa-fw'></i>".self::$locale['POLL_022']."</a></li>\n";

				}
				if (!empty($data['poll_ended']) && ($data['poll_ended'] < time())){
				echo "<li><a href='".FUSION_SELF.$aidlink."&amp;section=poll&amp;action=poll_unlock&amp;poll_id=".$data['poll_id']."'><i class='fa fa-trash fa-fw'></i>".self::$locale['POLL_023']."</a></li>\n";
				}
				if ($data['poll_started'] < time() && empty($data['poll_ended']) or $data['poll_ended'] > time()){
				echo "<li><a href='".FUSION_SELF.$aidlink."&amp;section=poll&amp;action=poll_lock&amp;poll_id=".$data['poll_id']."'><i class='fa fa-trash fa-fw'></i>".self::$locale['POLL_024']."</a></li>\n";
                }
				echo "<li class='divider'></li>\n";
				echo "<li><a href='".FUSION_SELF.$aidlink."&amp;section=poll_vote&amp;action=delete&amp;poll_id=".$data['poll_id']."'><i class='fa fa-trash fa-fw'></i> ".self::$locale['delete']."</a></li>\n";
                echo "</ul>\n";
                echo "</div>\n";
			echo "</div>\n";
		echo "</div>\n";
	echo "</div>\n";


			}
	echo "</div>\n";
		} else {
		echo "<div class='well text-center'>".self::$locale['POLL_012']."</div>\n";
		}
    }

    public function _archive_poll() {

    opentable(self::$locale['POLL_002']);

    $total_rows = $this->_countPoll("");

    $rowstart = isset($_GET['rowstart']) && ($_GET['rowstart'] <= $total_rows) ? $_GET['rowstart'] : 0;

	$result = $this->_selectDB($rowstart);

    $rows = dbrows($result);

    echo "<div class='clearfix'>\n";
    echo "<span class='pull-right m-t-10'>".sprintf(self::$locale['POLL_011'], $rows, $total_rows)."</span>\n";
    echo "</div>\n";

	echo ($total_rows > $rows) ? makepagenav($rowstart, self::$limit, $total_rows, self::$limit, FUSION_SELF."?") : "";


	if ($rows > 0) {
	echo "<div class='row m-t-20'>\n";
        while ($data = dbarray($result)) {
		$title = unserialize($data['poll_title']);
		$poll_opt = unserialize($data['poll_opt']);
		echo "<div class='col-xs-12 col-sm-3'>\n";
		echo "<p class='display-inline-block badge'>".self::$locale['POLL_044'].getgroupname($data['poll_visibility'])."</p>\n";

			echo "<div class='panel panel-default'>\n";
			echo "<div class='panel-heading text-left'>\n";
			foreach ($title as $key => $info) {
				echo "<p>".(!empty($info) ? translate_lang_names($key).": ".$info : $info)."</p>\n";
			}

			echo "</div>\n";

			echo "<div class='overflow-hide' >\n";

			$db_info = dbcount("(vote_opt)", DB_POLL_VOTES, "poll_id='".$data['poll_id']."'");
			foreach ($poll_opt as $keys => $data1) {
				$text = "";
				foreach ($data1 as $key => $inf) {
					$text .= "<p>".(!empty($inf) ? translate_lang_names($key).": ".$inf : $inf)."</p>\n";
					}

				$num_votes = dbcount("(vote_opt)", DB_POLL_VOTES, "vote_opt='".$keys."' AND poll_id='".$data['poll_id']."'");
    			$opt_votes = ($num_votes ? number_format(($num_votes / $db_info) * 100, 0) : number_format(0 * 100, 0));
				echo progress_bar($opt_votes,  $text,
									FALSE, // class
									FALSE,  // height
									FALSE,  // reverse
									TRUE,  // as percent
									FALSE
							);
				echo "<p><strong>".$opt_votes."% [".(empty($num_votes) ? 0 : $num_votes).($num_votes == 1 ? self::$locale['POLL_040'] : self::$locale['POLL_041'])."]</strong></p>\n";

				}
				echo "<p><strong>".self::$locale['POLL_060'].$db_info."</strong></p>\n";
				echo "<p>".self::$locale['POLL_061'].showdate("shortdate", $data['poll_started'])."</p>\n";
				echo "<p class='display-inline-block badge'>".self::$locale['POLL_064'].($data['poll_started'] > time() ? self::$locale['POLL_065'] : (!empty($data['poll_ended']) && ($data['poll_ended'] < time()) ? self::$locale['POLL_066'] : self::$locale['POLL_067']))."</p>\n";
					if (!empty($data['poll_ended']) && $data['poll_ended'] < time()) {
					echo "<p>".self::$locale['POLL_068'].showdate("shortdate", $data['poll_ended'])."</p>\n";
					}
				echo "</div>\n";
		echo "</div>\n";
	echo "</div>\n";


			}
	echo "</div>\n";
		} else {
		echo "<div class='well text-center'>".self::$locale['POLL_012']."</div>\n";
		}
	closetable();
    }

    private function set_polldb() {
	if (isset($_POST['cast_vote']) && (isset($_POST['poll_id']))) {
    $result = dbquery("SELECT v.vote_user, v.vote_id, v.vote_user_ip, v.vote_user_ip_type, p.poll_id, p.poll_opt, p.poll_started, p.poll_ended
		FROM ".DB_POLLS." p
		LEFT JOIN ".DB_POLL_VOTES." v ON p.poll_id = v.poll_id
		WHERE ".groupaccess('poll_visibility')." AND p.poll_id='".intval($_POST['poll_id'])."'
		ORDER BY v.vote_id");
        while ($pdata = dbarray($result)) {
            $voters[] = iMEMBER ? $pdata['vote_user'] : $pdata['vote_user_ip'];
            $data = $pdata;
        }
       if ($data['poll_started'] < time() && (empty($data['poll_ended']) or ($data['poll_ended'] > time())) && (empty($voters) || !in_array(fusion_get_userdata((iMEMBER ? 'user_id' : 'user_ip')),
                                                                                                           $voters)) && !empty($data["poll_opt"])
        ) {

    		$vote_save = array(
    		'vote_user' => iMEMBER ? fusion_get_userdata('user_id') : 0,
    		'vote_user_ip' => USER_IP,
    		'vote_user_ip_type' => USER_IP_TYPE,
    		'vote_opt' => stripinput($_POST['check']),
    		'poll_id' => intval($_POST['poll_id'])
    		);
        	if (\defender::safe()) {
    			dbquery_insert(DB_POLL_VOTES, $vote_save, "save");
				addNotice('success', "<i class='fa fa-check-square-o fa-lg m-r-10'></i>".self::$locale['POLL_013']);
        		}
        	} else {
			addNotice('warning', "<i class='fa fa-close fa-lg m-r-10'></i>".self::$locale['POLL_014']);

        	}
		redirect(FUSION_SELF.(FUSION_QUERY ? "?".FUSION_QUERY : ""));
    	}
    }

    public function poll_form() {
        fusion_confirm_exit();
	$this->data['poll_started'] = time();
    if ((isset($_GET['action']) && $_GET['action'] == "edit") && (isset($_GET['poll_id']))) {
        if (self::verify_poll($_GET['poll_id'])) {
    	$this->data = $this->_selectFormPoll(intval($_GET['poll_id']));
    	$this->data['poll_title'] = unserialize($this->data['poll_title']);
    	$this->data['poll_opt'] = unserialize($this->data['poll_opt']);
		$opt_count = isset($_POST['opt_count']) ? $_POST['opt_count'] : count($this->data['poll_opt']);
        }
    }

	if (isset($_POST['addoption'])) {

    $this->data['poll_title'] = stripinput($_POST['poll_title']);
    $this->data['poll_visibility'] = stripinput($_POST['poll_visibility']);
	$i=0;
		while ($i <$_POST['opt_count']) {

		    foreach ($_POST['poll_opt_'.$i] as $key => $value) {
	        $this->data['poll_opt'][$i][$key] = stripinput($_POST['poll_opt_'.$i][$key]);
		    }
	   $i++;
		}
	$this->data['poll_opt'][] = "";

	}

	$opt_count = count($this->data['poll_opt']);

    echo openform('addcat', 'post', FUSION_REQUEST, array('class' => "m-t-20"));
    echo form_hidden('poll_id', '', $this->data['poll_id']);
    echo form_hidden('opt_count', '', $opt_count);
    echo \PHPFusion\QuantumFields::quantum_multilocale_fields('poll_title', self::$locale['POLL_045'], $this->data['poll_title'], array(
                'required' => TRUE, 'inline' => TRUE, 'placeholder' => self::$locale['POLL_069'], 'inner_width'=>'250px'
            ));

	foreach ($this->data['poll_opt'] as $im1 => $data1) {
		$nam = "poll_opt_$im1";
    echo \PHPFusion\QuantumFields::quantum_multilocale_fields($nam, self::$locale['POLL_046'].$im1, $data1, array(
                'required' => TRUE, 'inline' => TRUE, 'placeholder' => self::$locale['POLL_070'], 'inner_width'=>'250px'
            ));
	}

    echo form_select('poll_visibility', self::$locale['POLL_047'], $this->data['poll_visibility'], array(
        "inline" => TRUE,
        'options' => fusion_get_groups()
    ));
    echo form_datepicker('poll_started', self::$locale['POLL_048'], $this->data['poll_started'], array('inline' => TRUE, 'inner_width'=>'250px'));
    echo form_datepicker('poll_ended', self::$locale['POLL_049'], $this->data['poll_ended'], array('inline' => TRUE, 'inner_width'=>'250px'));
    echo form_button('addoption', self::$locale['POLL_050'], self::$locale['POLL_050'], array(
        'class' => 'btn-primary',
        'inline' => 1
    ));
/*    echo form_button('preview', self::$locale['POLL_051'], self::$locale['POLL_051'], array(
        'class' => 'btn-primary',
        'inline' => 1
    ));*/
    echo form_button('save', self::$locale['POLL_052'], self::$locale['POLL_052'], array(
        'class' => 'btn-primary',
        'inline' => 1
    ));
    echo closeform();
    }

}