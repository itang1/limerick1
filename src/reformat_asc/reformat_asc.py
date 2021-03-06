import sys, os
from lists import *

def check_args():
    """
    checks for correct command-line inputs
    returns file stuff from the command line inputs
    """
    # check for correct command-line inputs
    if len(sys.argv) != 2:
        print ("usage: python reformat_asc.py [original_asc_filename input]")
        exit(-1)

    # the original asc input file
    original_asc_filename = sys.argv[1]
    return original_asc_filename

###################################
#### global variables here ########
###################################
# temporary buffer to store lines before writing them to reformat_asc file at the end
buffer = []
# check for correct command-line inputs, and initialize variables
original_asc_filename = check_args()
# metadata about the current trial
trial_metadatas = {
    'subtypeid' : '',
    'clashtype' : '',
    'secondarytask' : '',
    'dirtytype' : '',
    'iarea' : '',
    'old_trialid' : '',
    'buffer_holder_index_trialid_limerick' : -1,
    'buffer_holder_index_trialid_question' : -1,
    'buffer_holder_index_ias_limerick' : -1,
    'buffer_holder_index_eventsr' : -1,
    'buffer_holder_index_questionanswer' : -1,
    'camera_info' : [],
    'timestamp_end_lim' : '',
    'timestamp_end_ques'  : '',
    'timestamp_iarea' : '',
    'events_res_line' : '',
    'count' : 0,
    'synctime' : '0'
}
# ias for the questions
fake_question_ias = [ \
'MSG	00000001 REGION CHAR 0 1 j 254 790 260 829\n',
'MSG	00000001 DELAY 1 MS\n', \
'MSG	00000002 REGION CHAR 1 1 u 260 790 275 829\n', \
'MSG	00000002 DELAY 1 MS\n', \
'MSG	00000003 REGION CHAR 2 1 s 275 790 289 829\n', \
'MSG	00000003 DELAY 1 MS\n', \
'MSG	00000004 REGION CHAR 3 1 t 289 790 297 829\n', \
'MSG	00000004 DELAY 1 MS\n', \
'MSG	00000005 REGION CHAR 4 1   297 790 305 829\n', \
'MSG	00000005 DELAY 1 MS\n', \
'MSG	00000006 REGION CHAR 5 1 a 305 790 320 829\n', \
'MSG	00000006 DELAY 1 MS\n', \
'MSG	00000007 REGION CHAR 6 1   320 790 328 829\n', \
'MSG	00000007 DELAY 1 MS\n', \
'MSG	00000008 REGION CHAR 7 1 l 328 790 334 829\n', \
'MSG	00000008 DELAY 1 MS\n', \
'MSG	00000009 REGION CHAR 8 1 i 334 790 340 829\n', \
'MSG	00000009 DELAY 1 MS\n', \
'MSG	00000010 REGION CHAR 9 1 n 340 790 355 829\n', \
'MSG	00000010 DELAY 1 MS\n', \
'MSG	00000011 REGION CHAR 10 1 e 355 790 370 829\n', \
'MSG	00000011 DELAY 1 MS\n'
]

def getline(remaining_lines):
    """
    Removes the first item in the list, and returns it.
    The first item in the list corresponds to the line that is up next.
    remaining_lines is passed by reference
    """
    # get the next line; return the first item in the list of lines
    try:
        next_line = remaining_lines.pop(0)
        trial_metadatas['count'] = trial_metadatas.get('count') + 1
        return next_line

    # stop looping if the end of file is reached, write the contents of the buffer to the output file
    except IndexError:
        write_to_outfile(buffer)
        # print("Successfully parsed entire file.")
        exit(-1)

def open_input(original_asc_filename):
    """
    Opens the given input file.
    Parses it into a list, and returns that list.
    """
    try:
        infile = open(original_asc_filename, 'r')
        remaining_lines = infile.readlines()

        # skip everything after TRIALID 101
        indexes = [idx for idx, s in enumerate(remaining_lines) if 'TRIALID 101' in s]
        if len(indexes) > 0:
            remaining_lines = remaining_lines[:indexes[0]]

        infile.close()
        return remaining_lines

    except IOError:
        print("original_asc_filename file not found or path is incorrect")
        exit(-1)

def conversion_metadata(buffer, remaining_lines):
    """
    Processing the conversion metadata
    """
    # get the conversion metadata
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if line.strip() == '**':
            done = True
            buffer.append(line)
        # keep all blank lines
        elif line.strip() == '':
            buffer.append(line)
        # keep all comments, which begin with **
        elif line[0:2] == '**':
            buffer.append(line)

    # get the few lines before calibration info
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and '!CAL' in line:
            done = True
            buffer.append(line)
        # keep all blank lines
        elif line.strip() == '':
            buffer.append(line)
        # keep display coords info (should be just once)
        elif 'MSG' in line and 'DISPLAY_COORDS' in line:
            buffer.append(line)
        # NOTE seems like FRAMERATE is not applicable here -- see pg 83 of Eyelink Programmer Guide
        # NOTE not actually sure what retrace_interval is
        elif 'RETRACE_INTERVAL' in line:
            # pass
            buffer.append(line)
        # keep any input info
        elif 'INPUT' in line:
            buffer.append(line)

def calibration_validation(buffer, remaining_lines, recalibration):
    """
    Processing the calibration and validation info
    """
    # get the calibration and info
    # NOTE should we just keep the last successful calibration and validation, or keep all?
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and 'TRIALID' in line:
            done = True
            # if not recalibration, do not append here, will deal with this line in the next section (which  is trials)?
            if recalibration:
                trial_metadatas['old_trialid'] = line
                trial_metadatas['buffer_holder_index_trialid_limerick'] = len(buffer)
                buffer.append(line)
        else:
            buffer.append(line)
            # pass

def practice_trials(buffer, remaining_lines):
    """
    Process the practice trials by skipping them and the extra trial metadata
    """
    # skip the extra trial metadata info here
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and 'prepare_sequence' in line:
            done = True

def skip_to_next_trial(buffer, remaining_lines):
    """
    skip to where it says TRIALID number -- should just be the next line
    """
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and 'TRIALID' in line:
            done = True
            trial_metadatas['old_trialid'] = line
            trial_metadatas['buffer_holder_index_trialid_limerick'] = len(buffer)
            buffer.append(line)
        elif 'MSG' in line and '!CAL' in line:
            calibration_validation(buffer, remaining_lines, True)
            break

def read_camera_info(buffer, remaining_lines):
    """
    read in camera info
    """
    camera_info_copy = []
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)
        camera_info_copy.append(line)

        # increase timestamp for camera info, since timestamps from importing IAS overflows
        if 'RECCFG' in line or \
            'ELCLCFG' in line or \
            'GAZE_COORDS' in line or \
            'THRESHOLDS' in line or \
            'ELCL_WINDOW_SIZES' in line or \
            'CAMERA_LENS_FOCAL_LENGTH' in line or \
            'PUPIL_DATA_TYPE' in line or \
            'ELCL_PROC' in line or \
            'ELCL_PCR_PARAM' in line or \
            '!MODE RECORD' in line or \
            'START' in line:
            line_split = line.split(None, 2)
            timestamp = int(line_split[1])
            line = line_split[0] + ' ' + str(timestamp + 600) + ' ' + line_split[2]

        # exit state
        if '!MODE RECORD' in line:
            done = True
            try:
                buffer[buffer_holder_index_moder] = line
            except:
                pass
        elif 'RECCFG' in line:
            buffer_holder_index_input = len(buffer)
            # buffer.append("placeholder for INPUT")
            buffer.append(line)
        elif 'START' in line and 'EVENTS' in line:
            buffer_holder_index_moder = len(buffer)
            buffer.append("placeholder for !MODE RECORD")
            buffer.append(line)
        elif 'INPUT' in line and '127' in line:
            # buffer[buffer_holder_index_input] = line
            buffer.append(line)
        else:
            buffer.append(line)

    trial_metadatas['camera_info'] = camera_info_copy

def skip_dual_task_begin_instructions(buffer, remaining_lines):
    """
    skip the dual-task begin instructions screen
    including skip the three-ish lines between !MODE RECORD and START SECONDARY TASK
    includes gaze target on/off, i.e. the blue dot
    """

    # gaze target portion
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'BUTTON' in line and str(line.strip().split()[-1]) == '1':
            done = True
            buffer.append(line)
            buffer.append('MSG ' + line.split(None, 3)[1] +' GAZE TARGET ON\n')


    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and 'SHOW LIMERICK' in line:
            done = True
            line_split = line.split(None, 3)
            trial_metadatas['synctime'] = line_split[1]
            buffer.append(line_split[0] + ' ' + trial_metadatas.get('synctime') +' GAZE TARGET OFF\n')
            buffer.append(line_split[0] + ' ' + trial_metadatas.get('synctime') +' DISPLAY ON\n')
        elif 'IAREA FILE' in line:
            trial_metadatas['iarea'] = line
        elif 'MSG' in line and 'SYNCTIME' in line:
            line_split = line.split(None, 3)
            buffer.append(line_split[0] + ' ' + trial_metadatas.get('synctime') +' SYNCTIME\n')
        # collect eyetracking data, since we're still in gazetarget area
        elif 'SFIX' in line or \
                'EFIX' in line or \
                'SSACC' in line or \
                'ESACC' in line or \
                'SBLINK' in line or \
                'EBLINK' in line or \
                'BUTTON' in line:
            buffer.append(line)

def eye_movements_limerick(buffer, remaining_lines):
    """
    Parses the eye movements for viewing a single limerick
    """
    button_number = ''
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and 'STOP SECONDARY TASK' in line:
            done = True
            trial_metadatas['timestamp_end_lim'] = str(line.split()[1])
            # mark trial ok at the end of the limerick portion, add placeholders
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' ENDBUTTON ' + button_number + '\n')
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' DISPLAY OFF\n')
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' TRIAL_RESULT ' + button_number +'\n')
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' TRIAL OK\n')
            # put this placeholder here
            trial_metadatas['buffer_holder_index_eventsr'] = len(buffer)
            buffer.append('END ' + trial_metadatas.get('timestamp_end_lim') + '\n')

        # get eye movements
        elif 'SFIX' in line or \
                'EFIX' in line or \
                'SSACC' in line or \
                'ESACC' in line or \
                'SBLINK' in line or \
                'EBLINK' in line:
            buffer.append(line)
        elif 'BUTTON' in line:
            buffer.append(line)
            button_number = line.split()[2]

def skip_dual_task_end_instructions(buffer, remaining_lines):
    """
    Parses the dual-task end instructions screen (and the other trial metadata)
    """
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'MSG' in line and 'SHOW FOLLOWUP QUESTION' in line:
            done = True
        elif 'BUTTON' in line:
            buffer.append(line)

def question_placeholders(buffer, remaining_lines):
    """
    Placeholders for the TRIALID, .ias info
    to be tweaked or inserted later
    """
    # placeholder for TRIALID
    trial_metadatas['buffer_holder_index_trialid_question'] = len(buffer)
    buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' TRIALID\n')

    # placeholder index for the .ias stuff that will go here
    trial_metadatas['buffer_holder_index_questionanswer'] = len(buffer)
    buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' QUESTION_ANSWER\n')
    buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' DELAY 0 MS\n')
    buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' DISPLAY TEXT 1\n')

def question_cam_ias(buffer, remaining_lines):
    """
    insert camera info and fake ias info
    """
    # insert artificail aoi info, a single line at the bottom of the screen
    for line in fake_question_ias:
        line_split = line.split(None, 2)
        timestamp = int(line_split[1])
        line = line_split[0] + ' ' + str(int(trial_metadatas.get('timestamp_end_lim')) + timestamp) + ' ' + line_split[2]
        buffer.append(line)

    # insert camera info, with the correct timestamp
    for line in trial_metadatas.get('camera_info'):
        # increase timestamp for camera info, since timestamps from importing IAS overflows
        if 'RECCFG' in line or \
            'ELCLCFG' in line or \
            'GAZE_COORDS' in line or \
            'THRESHOLDS' in line or \
            'ELCL_WINDOW_SIZES' in line or \
            'CAMERA_LENS_FOCAL_LENGTH' in line or \
            'PUPIL_DATA_TYPE' in line or \
            'ELCL_PROC' in line or \
            'ELCL_PCR_PARAM' in line or \
            '!MODE RECORD' in line or \
            'START' in line:
            line_split = line.split(None, 2)
            timestamp = int(line_split[1])
            line = line_split[0] + ' ' + str(int(trial_metadatas.get('timestamp_end_lim')) + 300) + ' ' + line_split[2]
            buffer.append(line)
        else:
            buffer.append(line)

def eye_movements_question(buffer, remaining_lines):
    """
    Parses the eye movements for viewing a single question
    """

    buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' DISPLAY ON\n')
    buffer.append('MSG ' + trial_metadatas.get('timestamp_end_lim') + ' SYNCTIME\n')

    button_number = ''
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'END' in line and 'EVENTS' in line and 'RES' in line:
            done = True
            trial_metadatas['timestamp_end_ques'] = str(line.split()[1])
            # mark trial ok at the end of the limerick portion, add placeholders
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_ques') + ' ENDBUTTON ' + button_number + '\n')
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_ques') + ' DISPLAY OFF\n')
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_ques') + ' TRIAL_RESULT ' + button_number +'\n')
            buffer.append('MSG ' + trial_metadatas.get('timestamp_end_ques') + ' TRIAL OK\n')

            buffer.append(line)
            trial_metadatas['events_res_line'] = line

        # get eye movements
        elif 'SFIX' in line or \
                'EFIX' in line or \
                'SSACC' in line or \
                'ESACC' in line or \
                'SBLINK' in line or \
                'EBLINK' in line:
            buffer.append(line)
        elif 'BUTTON' in line:
            buffer.append(line)
            button_number = line.split()[2]

def parse_trial_var_metadata(buffer, remaining_lines):
    done = False
    while not done:
        # get the next line
        line = getline(remaining_lines)

        # exit state
        if 'TRIAL_RESULT' in line:
            done = True
        if 'TRIAL_VAR' in line and 'subtypeid' in line:
            trial_metadatas['subtypeid'] = line.split()[-1].strip()
        elif 'TRIAL_VAR' in line and 'clashtype' in line:
            trial_metadatas['clashtype'] = line.split()[-1].strip()
        elif 'TRIAL_VAR' in line and 'secondarytask' in line:
            trial_metadatas['secondarytask'] = line.split()[-1].strip()
        elif 'TRIAL_VAR' in line and 'dirtytype' in line:
            trial_metadatas['dirtytype'] = line.split()[-1].strip()
            if trial_metadatas.get('dirtytype') == 'dirty':
                trial_metadatas['dirtytype'] = '2'
            elif trial_metadatas.get('dirtytype') == 'clean':
                trial_metadatas['dirtytype'] = '3'

def tweak_stuff(buffer, remaining_lines):
    """
    Tweak info for the limerick and question.
    Stuff here should probably only be tweaking the buffer placeholders
    """
    # (for limerick) tweak the ID
    I = 'I' + str(trial_metadatas.get('subtypeid'))
    D = 'D0'
    if trial_metadatas.get('clashtype') == 'match' and trial_metadatas.get('secondarytask') == 'tap':
        E = 'E1'
    elif trial_metadatas.get('clashtype') == 'match' and trial_metadatas.get('secondarytask') == 'this':
        E = 'E2'
    elif trial_metadatas.get('clashtype') == 'clash' and trial_metadatas.get('secondarytask') == 'tap':
        E = 'E3'
    elif trial_metadatas.get('clashtype') == 'clash' and trial_metadatas.get('secondarytask') == 'this':
        E = 'E4'
    elif trial_metadatas.get('clashtype') == 'FILLLER':
        # NOTE the asc does indeed say filller with three L's
        E = 'E5'
        I = 'I' + str(int(trial_metadatas.get('subtypeid')) + 40)
    else:
        print("something broken with tweaking TRIALID")
        print(trial_metadatas.get('clashtype'), trial_metadatas.get('secondarytask'))
        exit(-1)

    EID = E + I + D
    new_id = trial_metadatas.get('old_trialid').rsplit(None, 1)[0] + ' ' + EID + '\n'
    buffer[trial_metadatas.get('buffer_holder_index_trialid_limerick')] = new_id


    # (for limerick) tweak the END EVENTS RES line
    buffer[trial_metadatas.get('buffer_holder_index_eventsr')] = 'END ' + trial_metadatas.get('timestamp_end_lim') + ' ' + trial_metadatas.get('events_res_line').split(None, 2)[2]

    # (for question) update the dirtytype question_answer
    buffer[trial_metadatas.get('buffer_holder_index_questionanswer')]  = buffer[trial_metadatas.get('buffer_holder_index_questionanswer')].strip() + ' ' + trial_metadatas.get('dirtytype') + '\n'

    # (for question) add + tweak the TRIALID
    EID = 'E100' + I + 'D1'
    buffer[trial_metadatas.get('buffer_holder_index_trialid_question')] = buffer[trial_metadatas.get('buffer_holder_index_trialid_question')].strip() + ' ' + EID + '\n'

def read_ias_letter(line, timestamp):
    """
    Reads the contentes of the .ias file letter-by-letter into a buffer
    (This method is not currently being used)
    """
    # format the path to the folder where the .ias files are stored
    # ias_folder = original_asc_filename.split('.')[0] + '_aoi'
    ias_folder = ''

    for list in ['A', 'B', 'C', 'D', 'RevA', 'RevB', 'RevC', 'RevD']:
        if str(original_asc_filename.split('/')[-1].split('.')[0]) in lists.get(list, 0):
            ias_folder = original_asc_filename.rsplit('/', 1)[0] + '/' + list + '_aoi'
            break
    if ias_folder == '':
        print ('unable to determine list')
        exit(-1)

    line = ias_folder + '/' + line.split('/')[-1].strip()
    iasfile = open(line, 'r')

    # buffer to hold the lines before they get combined into
    buffer_slice = []
    # position of the current character on the current line
    char_number = 0
    # line number that the current word appears on
    line_number = 0

    # include this for each limerick
    buffer_slice.append('MSG ' + str(timestamp) + ' DISPLAY TEXT 1\n')

    while True:
        # get the next line
        line = iasfile.readline()

        # return the buffer when the end of the .ias file is reached
        if not line:
            return buffer_slice

        # decompose the line
        line = line.split()
        word = line[-1]
        x_start = int(line[3])
        x_end = int(line[5])
        y_start = int(line[4])
        y_end = int(line[6])
        x_step = int((x_end - x_start)/(len(word) + 1))
        sequence = int(line[2])

        # increment the line number when appropriate
        if sequence == 1:
            line_number += 1
            char_number = 0

        # FIXME: spacing
        # evenly slice up the given coordinates across each character
        for i in range(len(word)+1):
            if i == len(word):
                c = ' '
                x = x_end
            else:
                c = word[i]
                x = x_start + x_step

            # add to buffer
            buffer_slice.append('MSG ' + str(timestamp) + ' REGION CHAR ' + str(char_number) + ' ' + str(line_number) + ' ' + c + ' ' + str(x_start) + ' ' + str(y_start) + ' ' + str(x) + ' ' + str(y_end) + '\n')
            buffer_slice.append('MSG ' + str(timestamp) + ' DELAY 1 MS' + '\n')
            x_start = x_start + x_step

            # increment timestamp and char_number
            timestamp += 1
            char_number += 1

def insert_ias(buffer, remaining_lines):
    """
    Insert IAS info about coordinates and letters display
    Do this last because it's inserting elements into the buffer, and not just amending existing elements
    """
    # (for limerick) insert info from .ias file into the stored buffer_holder_index_ias_limerick
    timestamp = int(trial_metadatas.get('old_trialid').rsplit()[1]) + 1
    # timestamp = int(trial_metadatas.get('iarea').split()[1])
    ias_info = read_ias_letter(trial_metadatas.get('iarea'), timestamp)
    buffer[trial_metadatas.get('buffer_holder_index_ias_limerick'):trial_metadatas.get('buffer_holder_index_ias_limerick')] = ias_info

def write_to_outfile(buffer):
    """
    Writes the contentes of the buffer list into the reformat_asc output file
    """
    # print original_asc_filename.rsplit('/', 2)
    inpath = original_asc_filename.rsplit('/', 2)
    reformat_asc_filename = inpath[0] + '/reformatted_asc/' + inpath[-1].split('.')[0] + '_reformatted.asc'

    # print reformat_asc_filename
    with open(reformat_asc_filename, 'w+') as outfile:
        for x in buffer:
            outfile.write(x)
    outfile.close()

def main():
    """
    Converts the .asc files produced by ExperimentBuilder into a format that can be
    parsed by UMass Eyetracking clean-up software
    """

    # open the input file, if possible, and read it into a list
    remaining_lines = open_input(original_asc_filename)

    # the current line that is being examined
    line = ''

    # METADATA, CALIBRATION, AND VALIDATION
    conversion_metadata(buffer, remaining_lines)
    calibration_validation(buffer, remaining_lines, False)

    # PRACTICE TRIALS -- basically skipping them
    practice_trials(buffer, remaining_lines)

    ############################################
    ### REAL TRIALS ############################
    ############################################

    while True:

        #### the limerick ###########################

        # parsing info for one trial
        # trigger: prepare_sequence
        skip_to_next_trial(buffer, remaining_lines)

        # placeholder index for the .ias stuff that will go here
        trial_metadatas['buffer_holder_index_ias_limerick'] = len(buffer)

        # read in camera info
        read_camera_info(buffer, remaining_lines)

        # skip the dual-task begin instructions screen
        skip_dual_task_begin_instructions(buffer, remaining_lines)

        # get the eye-movements for viewing the limerick
        eye_movements_limerick(buffer, remaining_lines)

        # skip the dual-task end instructions screen (and the other trial metadata)
        skip_dual_task_end_instructions(buffer, remaining_lines)

        #### the question ##########################

        # question placeholders
        question_placeholders(buffer, remaining_lines)

        # - add camera info and ias info
        question_cam_ias(buffer, remaining_lines)

        # get the eye-movements for viewing the question
        eye_movements_question(buffer, remaining_lines)

        # parse rest for trial metadata
        parse_trial_var_metadata(buffer, remaining_lines)

        # tweak limerick and question
        tweak_stuff(buffer, remaining_lines)

        # insert ias stuff for limerick and question
        insert_ias(buffer, remaining_lines)


# run main()
main()
