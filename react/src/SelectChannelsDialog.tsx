import { useState } from 'react';
import Button from '@material-ui/core/Button';
import List from '@material-ui/core/List';
import ListItem from '@material-ui/core/ListItem';
import ListItemText from '@material-ui/core/ListItemText';
import DialogTitle from '@material-ui/core/DialogTitle';
import DialogActions from '@material-ui/core/DialogActions';
import Dialog from '@material-ui/core/Dialog';
import FormControlLabel from '@material-ui/core/FormControlLabel';

import Checkbox from '@material-ui/core/Checkbox';



export interface SelectChannelsDialogProps {
    open: boolean;
    selectedChannels: string[];
    availableChannels: string[];
    onClose: (selectedChannels: string[] | null) => void;
}

function isChecked(selectedChannels: string[], channel: string): boolean {
    for (let i = 0; i < selectedChannels.length; ++i) {
        if (selectedChannels[i] === channel) return true;
    }
    return false;
}
function addPort(availableChannels: string[], selectedChannels: string[], newChannel: string) {
    let result: string[] = [];
    for (let i = 0; i < availableChannels.length; ++i) {
        let channel = availableChannels[i];
        if (isChecked(selectedChannels, channel) || channel === newChannel) {
            result.push(channel);
        }
    }
    return result;
}

function removePort(selectedChannels: string[], channel: string) {
    let result: string[] = [];
    for (let i = 0; i < selectedChannels.length; ++i) {
        if (selectedChannels[i] !== channel) {
            result.push(selectedChannels[i]);
        }
    }
    return result;
}

function SelectChannelsDialog(props: SelectChannelsDialogProps) {
    //const classes = useStyles();
    const { onClose, selectedChannels, availableChannels, open } = props;
    const [currentSelection, setCurrentSelection] = useState(selectedChannels);


    let showCheckboxes = availableChannels.length > 2;
    if (showCheckboxes) {

        let toggleSelect = (value: string) => {
            if (!isChecked(currentSelection, value)) {
                setCurrentSelection(addPort(availableChannels, currentSelection, value));
            } else {
                setCurrentSelection(removePort(currentSelection, value));
            }
        };

        const handleClose = (): void => {
            onClose(null);
        };
        const handleOk = (): void => {
            onClose(currentSelection);
        };


        return (
            <Dialog onClose={handleClose} aria-labelledby="select-channels-title" open={open}>
                <DialogTitle id="simple-dialog-title">Select Channels</DialogTitle>
                <List>
                    {availableChannels.map((channel) => (
                        <ListItem button >
                            <FormControlLabel
                                control={
                                    <Checkbox checked={isChecked(currentSelection, channel)} onClick={() => toggleSelect(channel)} key={channel} />
                                }
                                label={channel}
                            />
                        </ListItem>
                    )

                    )}
                </List>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">
                        Cancel
                    </Button>
                    <Button onClick={handleOk} color="primary" disabled={currentSelection.length === 0 || currentSelection.length > 2} >
                        OK
                    </Button>
                </DialogActions>
            </Dialog>
        );

    } else {

        const handleClose = () => {
            onClose(selectedChannels);
        };

        const handleListItemClick = (value: string) => {
            switch (value) {
                case "Stereo":
                    onClose(availableChannels);
                    break;
                case "Left":
                    onClose([availableChannels[0]]);
                    break;
                case "Right":
                    onClose([availableChannels[1]]);
                    break;
                case "None":
                default:
                    onClose([availableChannels[1]]);
            }
        };
        let selectionKey = "None";
        if (currentSelection.length === 2) {
            selectionKey = "Stereo";
        } else if (currentSelection.length === 1) {
            if (currentSelection[0] === availableChannels[0]) {
                selectionKey = "Left";
            } else {
                selectionKey = "Right";
            }
        }

        return (
            <Dialog onClose={handleClose} aria-labelledby="select-channels-title" open={open}>
                
                <List style={{ marginLeft: 0, marginRight: 0}}>
                    <ListItem button onClick={() => handleListItemClick("Stereo")} key={"Stereo"} selected={selectionKey === "Stereo"} >
                        <ListItemText primary={"Stereo"} />
                    </ListItem>
                    <ListItem button onClick={() => handleListItemClick("Left")} key={"LeftChannel"} selected={selectionKey === "Left"} >
                        <ListItemText primary={"Mono (left channel only)"} />
                    </ListItem>
                    <ListItem button onClick={() => handleListItemClick("Right")} key={"RightChannel"} selected={selectionKey === "Right"}>
                        <ListItemText primary={"Mono (right channel only"} />
                    </ListItem>
                </List>
            </Dialog>
        );
    }
}

export default SelectChannelsDialog;
