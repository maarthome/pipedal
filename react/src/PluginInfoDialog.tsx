import React from 'react';
import { createStyles, Theme, withStyles, WithStyles } from '@material-ui/core/styles';
import Button from '@material-ui/core/Button';
import Dialog from '@material-ui/core/Dialog';
import MuiDialogTitle from '@material-ui/core/DialogTitle';
import MuiDialogContent from '@material-ui/core/DialogContent';
import MuiDialogActions from '@material-ui/core/DialogActions';
import IconButton from '@material-ui/core/IconButton';
import CloseIcon from '@material-ui/icons/Close';
import Typography from '@material-ui/core/Typography';
import Grid from '@material-ui/core/Grid';
import { PiPedalModelFactory } from "./PiPedalModel";
import InfoOutlinedIcon from '@material-ui/icons/InfoOutlined';
import { UiPlugin, UiControl } from './Lv2Plugin';
import PluginIcon from './PluginIcon';


const styles = (theme: Theme) =>
    createStyles({
        root: {
            margin: 0,
            padding: theme.spacing(2),
        },
        closeButton: {
            position: 'absolute',
            right: theme.spacing(1),
            top: theme.spacing(1),
            color: theme.palette.grey[500],
        },
    });

export interface PluginInfoDialogTitleProps extends WithStyles<typeof styles> {
    id: string;
    children: React.ReactNode;
    onClose: () => void;
}

// const PluginInfoDialogTitle = withStyles(styles)((props: PluginInfoDialogTitleProps) => {
//     const { children, classes, onClose, ...other } = props;
//     return (
//         <MuiDialogTitle style={{ display: "flex", flexDirection: "row", alignItems: "start", flexWrap: "nowrap" }}>
//             <div style={{ flex: "0 0 auto", marginRight: 8 }}>
//                 <PluginIcon pluginType={plugin.plugin_type} pluginUri={plugin.uri} offsetY={5} />
//             </div>
//             <div style={{ flex: "1 1 auto" }}>
//                 {children}
//             </div>
//             <IconButton aria-label="close" className={classes.closeButton} onClick={() => handleClose()}
//                 style={{ flex: "0 0 auto" }}>
//                 <CloseIcon />
//             </IconButton>
//         </MuiDialogTitle>
//     );
// });

const PluginInfoDialogContent = withStyles((theme: Theme) => ({
    root: {
        padding: theme.spacing(2),
    },
}))(MuiDialogContent);

const PluginInfoDialogActions = withStyles((theme: Theme) => ({
    root: {
        margin: 0,
        padding: theme.spacing(1),
    },
}))(MuiDialogActions);

export interface PluginInfoProps extends WithStyles<typeof styles> {
    plugin_uri: string
}

function displayChannelCount(count: number): string {
    if (count === 0) return "None";
    if (count === 1) return "Mono";
    if (count === 2) return "Stereo";
    return count + " channels";
}
function ioDescription(plugin: UiPlugin): string {
    let result = "Input: " + displayChannelCount(plugin.audio_inputs) + ". Output: " + displayChannelCount(plugin.audio_outputs) + ".";
    if (plugin.has_midi_input) {
        if (plugin.has_midi_output) {
            result += " Midi in/out.";
        } else {
            result += " Midi in.";
        }
    } else {
        if (plugin.has_midi_output) {
            result += "Midi out.";
        }
    }
    return result;


}

function makeParagraphs(description: string) {
    description = description.replaceAll('\r', '');
    description = description.replaceAll('\n\n', '\r');
    description = description.replaceAll('\n', ' ');

    let paragraphs: string[] = description.split('\r');
    return (
        <div style={{ paddingLeft: "24px" }}>
            {paragraphs.map((para) => (
                <Typography variant="body2" paragraph >
                    {para}
                </Typography>
            ))}
        </div>

    );
}
function makeControls(controls: UiControl[]) {
    return (
        <Grid container direction="row" justify="flex-start" alignItems="flex-start" spacing={1} style={{ paddingLeft: "24px" }}>
            {
                controls.map((control) => (
                    <Grid item spacing={2} xs={6} sm={4} key={control.symbol} >
                        <Typography variant="body2">
                            {control.name}
                        </Typography>
                    </Grid>
                ))
            }
        </Grid>

    );

}

const PluginInfoDialog = withStyles(styles)((props: PluginInfoProps) => {

    let model = PiPedalModelFactory.getInstance();
    const [open, setOpen] = React.useState(false);
    let { classes, plugin_uri } = props;

    const handleClickOpen = () => {
        setOpen(true);
    };
    const handleClose = () => {
        setOpen(false);
    };
    let uri = props.plugin_uri;
    let visible = true;
    if (uri === null || uri === "") {
        visible = false;
    }
    if (!visible) {
        return (<div></div>);
    };
    let plugin = model.getUiPlugin(plugin_uri);

    if (plugin === null) {
        return (<div></div>)
    }

    return (
        <div>
            <IconButton style={{ display: (props.plugin_uri !== "") ? "block" : "none" }} onClick={handleClickOpen}
            >
                <InfoOutlinedIcon />
            </IconButton>
            {open && (
                <Dialog onClose={handleClose} open={open} fullWidth >
                    <MuiDialogTitle >
                        <div style={{ display: "flex", flexDirection: "row", alignItems: "start", flexWrap: "nowrap" }}>
                            <div style={{ flex: "0 0 auto", marginRight: 16 }}>
                                <PluginIcon pluginType={plugin.plugin_type} pluginUri={plugin.uri} offsetY={3} />
                            </div>
                            <div style={{ flex: "1 1 auto" }}>
                                <Typography variant="h6">{plugin.name}</Typography>
                            </div>
                            <IconButton aria-label="close" className={classes.closeButton} onClick={() => handleClose()}
                                style={{ flex: "0 0 auto" }}>
                                <CloseIcon />
                            </IconButton>
                        </div>
                    </MuiDialogTitle>
                    <PluginInfoDialogContent dividers style={{ width: "100%", maxHeight: "80%", overflowX: "hidden" }}>
                        <Typography gutterBottom>
                            Author:&nbsp;
                            {(plugin.author_homepage !== "")
                                ? <a href={plugin.author_homepage} target="_blank" rel="noreferrer">{plugin.author_name}</a>
                                : (
                                    plugin.author_name
                                )
                            }
                        </Typography>
                        <Typography gutterBottom>
                            {ioDescription(plugin)}
                        </Typography>

                        <Typography variant="body1" gutterBottom style={{ paddingTop: "1em" }}>
                            Controls:
                        </Typography>
                        {
                            makeControls(plugin.controls)
                        }
                        <Typography gutterBottom style={{ paddingTop: "1em" }}>
                            Description:
                        </Typography>
                        {
                            (plugin.description !== "") && makeParagraphs(plugin.description)
                        }
                    </PluginInfoDialogContent>
                    <PluginInfoDialogActions>
                        <Button autoFocus onClick={handleClose} color="primary" style={{ width: "130px" }}>
                            OK
                        </Button>
                    </PluginInfoDialogActions>
                </Dialog>
            )}
        </div >
    );
});

export default PluginInfoDialog;
