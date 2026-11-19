namespace HolderLinux {

public class GitProviderCatalogEntry : Object {
    public string id { get; construct; }
    public string name { get; construct; }
    public string kind { get; construct; }
    public string preferred_transport { get; construct; }
    public string transports_summary { get; construct; }
    public string ssh_example { get; construct; }
    public string https_example { get; construct; }

    public GitProviderCatalogEntry(string id,
                                   string name,
                                   string kind,
                                   string preferred_transport,
                                   string transports_summary,
                                   string ssh_example,
                                   string https_example) {
        Object(
            id: id,
            name: name,
            kind: kind,
            preferred_transport: preferred_transport,
            transports_summary: transports_summary,
            ssh_example: ssh_example,
            https_example: https_example
        );
    }
}

public class GitTestRemoteResult : Object {
    public string project_id { get; construct; }
    public string remote_url { get; construct; }
    public string status { get; construct; }
    public bool remote_has_head { get; construct; }
    public string error_message { get; construct; }

    public GitTestRemoteResult(string project_id,
                               string remote_url,
                               string status,
                               bool remote_has_head,
                               string error_message) {
        Object(
            project_id: project_id,
            remote_url: remote_url,
            status: status,
            remote_has_head: remote_has_head,
            error_message: error_message
        );
    }
}

// A push's outcome: its push phase, with the branch it was asked for.
public class GitPushResult : Object {
    public string project_id { get; construct; }
    public string remote_url { get; construct; }
    public string branch { get; construct; }
    public string status { get; construct; }
    public int ahead_count { get; construct; }
    public int behind_count { get; construct; }
    public string local_head_commit { get; construct; }
    public string error_message { get; construct; }
    public string next_action { get; construct; }

    public GitPushResult(string project_id,
                         string remote_url,
                         string branch,
                         string status,
                         int ahead_count,
                         int behind_count,
                         string local_head_commit,
                         string error_message,
                         string next_action) {
        Object(
            project_id: project_id,
            remote_url: remote_url,
            branch: branch,
            status: status,
            ahead_count: ahead_count,
            behind_count: behind_count,
            local_head_commit: local_head_commit,
            error_message: error_message,
            next_action: next_action
        );
    }
}

public class GitPullPhase : Object {
    public bool attempted { get; construct; }
    public string status { get; construct; }
    public int conflicts_resolved { get; construct; }
    public int hidden_cards_repaired { get; construct; }
    public string error_message { get; construct; }

    public GitPullPhase(bool attempted, string status, int conflicts_resolved,
                        int hidden_cards_repaired, string error_message) {
        Object(attempted: attempted, status: status, conflicts_resolved: conflicts_resolved,
               hidden_cards_repaired: hidden_cards_repaired, error_message: error_message);
    }
}

public class GitSyncResult : Object {
    public GitPullPhase pull { get; construct; }
    public bool push_attempted { get; construct; }
    public GitPushResult push { get; construct; }

    public GitSyncResult(GitPullPhase pull, bool push_attempted, GitPushResult push) {
        Object(pull: pull, push_attempted: push_attempted, push: push);
    }
}

public class GitPushDetails : ActivityDetails {
    public string status { get; construct; }
    public string local_head_commit { get; construct; }
    public string branch { get; construct; }

    public GitPushDetails(string status, string local_head_commit, string branch) {
        Object(status: status, local_head_commit: local_head_commit, branch: branch);
    }
}

}
